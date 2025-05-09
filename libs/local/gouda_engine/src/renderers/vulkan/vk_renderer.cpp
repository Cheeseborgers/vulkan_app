/**
 * @file renderers/vulkan/vk_renderer.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-28
 * @brief Engine module implementation
 */
#include "renderers/vulkan/vk_renderer.hpp"

#include <cstring>
#include <thread>

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "debug/debug.hpp"
#include "math/random.hpp"
#include "renderers/vulkan/vk_buffer_manager.hpp"
#include "renderers/vulkan/vk_command_buffer_manager.hpp"
#include "renderers/vulkan/vk_compute_pipeline.hpp"
#include "renderers/vulkan/vk_depth_resources.hpp"
#include "renderers/vulkan/vk_fence.hpp"
#include "renderers/vulkan/vk_graphics_pipeline.hpp"
#include "renderers/vulkan/vk_instance.hpp"
#include "renderers/vulkan/vk_shader.hpp"
#include "renderers/vulkan/vk_texture.hpp"
#include "renderers/vulkan/vk_utils.hpp"

#include "renderers/vulkan/gouda_vk_wrapper.hpp"

namespace gouda::vk {

// Renderer implentation --------------------------------------------
Renderer::Renderer()
    : p_instance{nullptr},
      p_device{nullptr},
      p_buffer_manager{nullptr},
      p_swapchain{nullptr},
      p_depth_resources{nullptr},
      p_command_buffer_manager{nullptr},
      p_quad_pipeline{nullptr},
      p_text_pipeline{nullptr},
      p_particle_pipeline{nullptr},
      p_particle_compute_pipeline{nullptr},
      p_quad_vertex_buffer{nullptr},
      p_quad_index_buffer{nullptr},
      p_quad_vertex_shader{nullptr},
      p_quad_fragment_shader{nullptr},
      p_text_vertex_shader{nullptr},
      p_text_fragment_shader{nullptr},
      p_particle_vertex_shader{nullptr},
      p_particle_fragment_shader{nullptr},
      p_particle_compute_shader{nullptr},
      p_window{nullptr},
      p_render_pass{VK_NULL_HANDLE},
      p_copy_command_buffer{VK_NULL_HANDLE},
      p_imgui_pool{VK_NULL_HANDLE},
      m_queue{},
      m_frame_fences{},
      m_framebuffers{},
      m_command_buffers{},
      m_uniform_buffers{},
      m_quad_instance_buffers{},
      m_mapped_quad_instance_data{},
      m_framebuffer_size{0, 0},
      m_current_frame{0},
      m_simulation_params{0.0f, {0.0f, -9.81f, 0.0f}},
      m_clear_colour{},
      m_max_quad_instances{1000},
      m_max_text_instances{1000},
      m_max_particle_instances{1000},
      m_vsync_mode{VSyncMode::Disabled},
      m_vertex_count{0},
      m_index_count{0},
      m_is_initialized{false},
      m_use_compute_particles{false},
      m_particles_instances{}
{
}

Renderer::~Renderer()
{
    ENGINE_LOG_DEBUG("Cleaning up Vulkan renderer.");

    if (m_is_initialized) {
        DestroyBuffers();

        for (auto &texture : m_textures) {
            texture->Destroy(p_device.get());
        }

        if (p_render_pass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(p_device->GetDevice(), p_render_pass, nullptr);
            p_render_pass = VK_NULL_HANDLE;
            ENGINE_LOG_DEBUG("Render pass destroyed.");
        }

        DestroyFramebuffers();
        DestroyImGUI();
    }
}

void Renderer::Initialize(GLFWwindow *window_ptr, std::string_view app_name, SemVer vulkan_api_version,
                          VSyncMode vsync_mode)
{
    ASSERT(window_ptr, "Window pointer cannot be null.");
    ASSERT(!app_name.empty(), "Application name cannot be empty or null.");

    ENGINE_PROFILE_SESSION("Gouda", "debug/profiling/results.json");
    ENGINE_PROFILE_SCOPE("Init renderer");

    m_vsync_mode = vsync_mode;
    m_particles_instances.clear();

    InitializeCore(window_ptr, app_name, vulkan_api_version);
    InitializeSwapchainAndQueue(vsync_mode);
    InitializeRenderResources();
    InitializeDefaultResources();
    InitializeImGUIIfEnabled();

    m_is_initialized = true;
    ENGINE_LOG_DEBUG("Renderer initialized.");
}

void Renderer::RecordCommandBuffer(VkCommandBuffer command_buffer, u32 image_index, u32 quad_instance_count,
                                   u32 text_instance_count, u32 particle_instance_count, ImDrawData *draw_data)
{
    ENGINE_PROFILE_SCOPE("Record command buffer");

    BeginCommandBuffer(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = m_clear_colour;
    clear_values[1].depthStencil = {1.0f, 0};

    VkExtent2D extent{p_swapchain->GetExtent()};
    VkRenderPassBeginInfo render_pass_begin_info{.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                                 .renderPass = p_render_pass,
                                                 .framebuffer = m_framebuffers[image_index],
                                                 .renderArea = {{0, 0}, extent},
                                                 .clearValueCount = static_cast<u32>(clear_values.size()),
                                                 .pClearValues = clear_values.data()};

    VkViewport viewport{.x = 0.0f,
                        .y = 0.0f,
                        .width = static_cast<f32>(extent.width),
                        .height = static_cast<f32>(extent.height),
                        .minDepth = 0.0f,
                        .maxDepth = 1.0f};

    VkRect2D scissor{{0, 0}, extent};

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    // Compute pipeline
    if (m_use_compute_particles && particle_instance_count > 0) {
        ENGINE_LOG_DEBUG("Compute dispatch: particle_count = {}", particle_instance_count);
        p_particle_compute_pipeline->Bind(command_buffer);
        p_particle_compute_pipeline->BindDescriptors(command_buffer, image_index);
        u32 particle_count{math::min(particle_instance_count, static_cast<u32>(m_max_particle_instances))};
        u32 workgroup_count{(particle_count + 255) / 256};
        p_particle_compute_pipeline->Dispatch(command_buffer, workgroup_count, 1, 1);

        VkBufferMemoryBarrier compute_to_copy_barrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = m_particle_storage_buffers[image_index].p_buffer,
            .offset = 0,
            .size = sizeof(ParticleData) * particle_count,
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 1, &compute_to_copy_barrier, 0, nullptr);

        VkBufferCopy copy_region{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = sizeof(ParticleData) * particle_count,
        };
        vkCmdCopyBuffer(command_buffer, m_particle_storage_buffers[image_index].p_buffer,
                        m_particle_instance_buffers[image_index].p_buffer, 1, &copy_region);

        VkBufferMemoryBarrier copy_to_vertex_barrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = m_particle_instance_buffers[image_index].p_buffer,
            .offset = 0,
            .size = sizeof(ParticleData) * particle_count,
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0,
                             nullptr, 1, &copy_to_vertex_barrier, 0, nullptr);

        ParticleData *instance_particles = static_cast<ParticleData *>(m_mapped_particle_instance_data[image_index]);
        /*
    if (particle_count > 0) {

        ENGINE_LOG_DEBUG(
            "After copy[{}]: Particle[0]: pos = [{}, {}, {}], size = [{}, {}], lifetime = {}, vel = [{}, {}, {}], "
            "colour = [{}, {}, {}, {}]",
            image_index, instance_particles[0].position[0], instance_particles[0].position[1],
            instance_particles[0].position[2], instance_particles[0].size[0], instance_particles[0].size[1],
            instance_particles[0].lifetime, instance_particles[0].velocity[0], instance_particles[0].velocity[1],
            instance_particles[0].velocity[2], instance_particles[0].colour[0], instance_particles[0].colour[1],
            instance_particles[0].colour[2], instance_particles[0].colour[3]);


        }
        if (particle_count > 2) {

            ENGINE_LOG_DEBUG(
                "After copy[{}]: Particle[2]: pos = [{}, {}, {}], size = [{}, {}], lifetime = {}, vel = [{}, {}, {}], "
                "colour = [{}, {}, {}, {}]",
                image_index, instance_particles[2].position[0], instance_particles[2].position[1],
                instance_particles[2].position[2], instance_particles[2].size[0], instance_particles[2].size[1],
                instance_particles[2].lifetime, instance_particles[2].velocity[0], instance_particles[2].velocity[1],
                instance_particles[2].velocity[2], instance_particles[2].colour[0], instance_particles[2].colour[1],
                instance_particles[2].colour[2], instance_particles[2].colour[3]);

        }
                */
    }

    // Quad/Sprite rendering
    if (quad_instance_count > 0) {
        p_quad_pipeline->Bind(command_buffer, image_index);
        VkBuffer buffers[]{p_quad_vertex_buffer->p_buffer, m_quad_instance_buffers[image_index].p_buffer};
        VkDeviceSize offsets[]{0, 0};
        vkCmdBindVertexBuffers(command_buffer, 0, 2, buffers, offsets);
        vkCmdBindIndexBuffer(command_buffer, p_quad_index_buffer->p_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer, m_index_count, quad_instance_count, 0, 0, 0);
    }

    // Text rendering
    if (text_instance_count > 0 && p_text_pipeline) {
        p_text_pipeline->Bind(command_buffer, image_index);
        VkBuffer buffers[]{p_quad_vertex_buffer->p_buffer, m_text_instance_buffers[image_index].p_buffer};
        VkDeviceSize offsets[]{0, 0};
        vkCmdBindVertexBuffers(command_buffer, 0, 2, buffers, offsets);
        vkCmdBindIndexBuffer(command_buffer, p_quad_index_buffer->p_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer, m_index_count, text_instance_count, 0, 0, 0);
    }

    // Particle rendering
    if (p_particle_pipeline && particle_instance_count > 0) {
        ENGINE_LOG_DEBUG("Particle draw: instance_count = {}", particle_instance_count);
        p_particle_pipeline->Bind(command_buffer, image_index);
        VkBuffer buffers[]{p_quad_vertex_buffer->p_buffer, m_particle_instance_buffers[image_index].p_buffer};
        VkDeviceSize offsets[]{0, 0};
        vkCmdBindVertexBuffers(command_buffer, 0, 2, buffers, offsets);
        vkCmdBindIndexBuffer(command_buffer, p_quad_index_buffer->p_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer, m_index_count, particle_instance_count, 0, 0, 0);
    }

    // Render ImGui
    if (draw_data) {
        ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
    }

    vkCmdEndRenderPass(command_buffer);
    EndCommandBuffer(command_buffer);
}

void Renderer::Render(f32 delta_time, const UniformData &uniform_data, const std::vector<InstanceData> &quad_instances,
                      const std::vector<TextData> &text_instances, const std::vector<ParticleData> &particle_instances)
{
    // ENGINE_LOG_DEBUG("Render: particle_instances.size() = {}", particle_instances.size());
    for (size_t i = 0; i < std::min(particle_instances.size(), size_t(3)); ++i) {
        /*
        ENGINE_LOG_DEBUG("Particle[{}]: pos = [{}, {}, {}], size = [{}, {}], lifetime = {}, colour = [{}, {}, {}, {}]",
                         i, particle_instances[i].position[0], particle_instances[i].position[1],
                         particle_instances[i].position[2], particle_instances[i].size[0],
                         particle_instances[i].size[1], particle_instances[i].lifetime, particle_instances[i].colour[0],
                         particle_instances[i].colour[1], particle_instances[i].colour[2],
                         particle_instances[i].colour[3]);
        */
    }

    while (!p_swapchain->IsValid()) {
        std::this_thread::yield();
        ENGINE_LOG_INFO("Rendering paused, waiting for valid swapchain");
    }

    m_frame_fences[m_current_frame].WaitFor(Constants::u64_max);
    m_frame_fences[m_current_frame].Reset();

    u32 image_index{m_queue.AcquireNextImage(m_current_frame)};
    if (image_index == Constants::u32_max) {
        return;
    }

    m_particles_instances = particle_instances;

    // Update uniform buffer
    m_uniform_buffers[image_index].Update(p_device->GetDevice(), &uniform_data, sizeof(uniform_data));

    // Update quad instance data
    VkDeviceSize quad_instance_size{sizeof(InstanceData) * quad_instances.size()};
    ASSERT(quad_instance_size <= sizeof(InstanceData) * m_max_quad_instances,
           "Quad instance count exceeds maximum buffer size.");
    if (quad_instance_size > 0) {
        memcpy(m_mapped_quad_instance_data[image_index], quad_instances.data(), quad_instance_size);
    }

    // Update text instance data
    VkDeviceSize text_instance_size{sizeof(TextData) * text_instances.size()};
    ASSERT(text_instance_size <= sizeof(TextData) * m_max_text_instances,
           "Text instance count exceeds maximum buffer size.");
    if (text_instance_size > 0) {
        memcpy(m_mapped_text_instance_data[image_index], text_instances.data(), text_instance_size);
    }

    // Update particle buffers
    UpdateComputeUniformBuffer(image_index, delta_time);
    UpdateParticleStorageBuffer(image_index, particle_instances);
    if (!m_use_compute_particles && !particle_instances.empty()) {
        VkDeviceSize particle_instance_size{sizeof(ParticleData) * particle_instances.size()};
        ASSERT(particle_instance_size <= sizeof(ParticleData) * m_max_particle_instances,
               "Particle instance count exceeds maximum buffer size.");
        memcpy(m_mapped_particle_instance_data[image_index], particle_instances.data(), particle_instance_size);
    }
    else if (m_use_compute_particles && !particle_instances.empty()) {
        // Update m_particles_instances with compute results
        ParticleData *storage_particles{static_cast<ParticleData *>(m_mapped_particle_storage_data[image_index])};
        std::vector<ParticleData> updated_particles;
        updated_particles.reserve(particle_instances.size());
        for (size_t i = 0; i < particle_instances.size(); ++i) {
            if (storage_particles[i].lifetime > 0.0f && storage_particles[i].size.x > 0.0f &&
                storage_particles[i].size.y > 0.0f) {
                updated_particles.push_back(storage_particles[i]);
            }
        }
        m_particles_instances = updated_particles;
        ENGINE_LOG_DEBUG("After compute: m_particles_instances.size() = {}", m_particles_instances.size());
    }

    // Render ImGui
    ImDrawData *draw_data{nullptr};
#ifdef USE_IMGUI
    RenderStatistics stats{.delta_time = delta_time,
                           .instance_count = static_cast<u32>(quad_instances.size()),
                           .vertex_count = m_vertex_count,
                           .index_count = m_index_count,
                           .particle_count = static_cast<u32>(m_particles_instances.size()),
                           .glyph_count = static_cast<u32>(text_instances.size()),
                           .texture_count = static_cast<u32>(m_textures.size())};
    draw_data = RenderImGUI(stats);
#endif

    vkResetCommandBuffer(m_command_buffers[image_index], 0);
    RecordCommandBuffer(m_command_buffers[image_index], image_index, static_cast<u32>(quad_instances.size()),
                        static_cast<u32>(text_instances.size()), static_cast<u32>(m_particles_instances.size()),
                        draw_data);

    m_queue.Submit(m_command_buffers[image_index], m_current_frame, &m_frame_fences[m_current_frame]);
    m_queue.Present(image_index, m_current_frame);

    m_current_frame = (m_current_frame + 1) % m_queue.GetMaxFramesInFlight();
}

void Renderer::UpdateComputeUniformBuffer(u32 image_index, f32 delta_time)
{
    m_simulation_params.deltaTime = delta_time;
    m_compute_uniform_buffers[image_index].Update(p_device->GetDevice(), &m_simulation_params,
                                                  sizeof(SimulationParams));
}

void Renderer::UpdateParticleStorageBuffer(u32 image_index, const std::vector<ParticleData> &particle_instances)
{
    VkDeviceSize max_particle_instance_size{sizeof(ParticleData) * m_max_particle_instances};

    memset(m_mapped_particle_storage_data[image_index], 0, max_particle_instance_size);

    if (particle_instances.empty()) {
        ClearParticleBuffers(image_index);
        return;
    }

    VkDeviceSize particle_instance_size{
        sizeof(ParticleData) * std::min(particle_instances.size(), static_cast<size_t>(m_max_particle_instances))};
    memcpy(m_mapped_particle_storage_data[image_index], particle_instances.data(), particle_instance_size);

    ParticleData *gpu_particles{static_cast<ParticleData *>(m_mapped_particle_storage_data[image_index])};
    /*
    if (particle_instances.size() > 0) {
        ENGINE_LOG_DEBUG("Storage buffer[{}] Particle[0]: pos = [{}, {}, {}], size = [{}, {}], lifetime = {}, vel = "
                         "[{}, {}, {}], colour = [{}, {}, {}, {}]",
                         image_index, gpu_particles[0].position[0], gpu_particles[0].position[1],
                         gpu_particles[0].position[2], gpu_particles[0].size[0], gpu_particles[0].size[1],
                         gpu_particles[0].lifetime, gpu_particles[0].velocity[0], gpu_particles[0].velocity[1],
                         gpu_particles[0].velocity[2], gpu_particles[0].colour[0], gpu_particles[0].colour[1],
                         gpu_particles[0].colour[2], gpu_particles[0].colour[3]);
    }
    if (particle_instances.size() > 2) {
        ENGINE_LOG_DEBUG("Storage buffer[{}] Particle[2]: pos = [{}, {}, {}], size = [{}, {}], lifetime = {}, vel = "
                         "[{}, {}, {}], colour = [{}, {}, {}, {}]",
                         image_index, gpu_particles[2].position[0], gpu_particles[2].position[1],
                         gpu_particles[2].position[2], gpu_particles[2].size[0], gpu_particles[2].size[1],
                         gpu_particles[2].lifetime, gpu_particles[2].velocity[0], gpu_particles[2].velocity[1],
                         gpu_particles[2].velocity[2], gpu_particles[2].colour[0], gpu_particles[2].colour[1],
                         gpu_particles[2].colour[2], gpu_particles[2].colour[3]);
    }
                         */
}

void Renderer::ClearParticleBuffers(u32 image_index)
{
    VkDeviceSize max_particle_instance_size = sizeof(ParticleData) * m_max_particle_instances;
    memset(m_mapped_particle_storage_data[image_index], 0, max_particle_instance_size);
    memset(m_mapped_particle_instance_data[image_index], 0, max_particle_instance_size);
}

void Renderer::ToggleComputeParticles()
{
    m_use_compute_particles = !m_use_compute_particles;
    ENGINE_LOG_DEBUG("Compute particles {}", m_use_compute_particles ? "enabled" : "disabled");
}

void Renderer::DrawText(std::string_view text, Vec2 position, f32 scale, u32 font_id,
                        std::vector<TextData> &text_instances)
{
    if (m_fonts.find(font_id) == m_fonts.end()) {
        ENGINE_LOG_ERROR("Font ID {} not found!", font_id);
        return;
    }

    const auto &glyphs = m_fonts[font_id];
    f32 x = position.x;
    f32 y = position.y;

    ENGINE_LOG_DEBUG("Drawing text '{}' at position=({}, {}), scale={}, font_id={}", text, x, y, scale, font_id);
    ENGINE_LOG_DEBUG("Available glyphs: {}", glyphs.size());
    for (const auto &[c, _] : glyphs) {
        ENGINE_LOG_DEBUG("Glyph '{}'(unicode={})", c, static_cast<int>(c));
    }

    size_t instance_count = text_instances.size();
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        auto it = glyphs.find(c);
        if (it == glyphs.end()) {
            ENGINE_LOG_WARNING("Glyph '{}' (unicode={}) not found in font {}", c, static_cast<int>(c), font_id);
            continue;
        }

        const Glyph &glyph = it->second;

        // Log glyph data
        ENGINE_LOG_DEBUG("Glyph {} '{}': advance={}, plane_bounds=({}, {}, {}, {}), atlas_bounds=({}, {}, {}, {})", i,
                         c, glyph.advance, glyph.plane_bounds.left, glyph.plane_bounds.bottom, glyph.plane_bounds.right,
                         glyph.plane_bounds.top, glyph.atlas_bounds.left, glyph.atlas_bounds.bottom,
                         glyph.atlas_bounds.right, glyph.atlas_bounds.top);

        // Compute bounds
        f32 left = std::min(glyph.plane_bounds.left, glyph.plane_bounds.right);
        f32 right = std::max(glyph.plane_bounds.left, glyph.plane_bounds.right);
        f32 bottom = std::min(glyph.plane_bounds.bottom, glyph.plane_bounds.top);
        f32 top = std::max(glyph.plane_bounds.bottom, glyph.plane_bounds.top);

        f32 xpos = x + left * scale;
        f32 ypos = y + bottom * scale;
        f32 width = (right - left) * scale;
        f32 height = (top - bottom) * scale;

        TextData instance;
        instance.position = Vec3{xpos, ypos, -0.1f};
        instance.size = Vec2{width, height};
        instance.colour = Vec4{1.0f, 0.0f, 0.0f, 1.0f}; // Red for visibility
        instance.glyph_index = static_cast<u32>(c);
        instance.sdf_params =
            Vec4{glyph.atlas_bounds.left, glyph.atlas_bounds.bottom, glyph.atlas_bounds.right, glyph.atlas_bounds.top};
        instance.texture_index = font_id;

        // Log instance data
        ENGINE_LOG_DEBUG("Instance {} '{}': position=({}, {}, {}), size=({}, {}), glyph_index={}, sdf_params=({}, {}, "
                         "{}, {}), texture_index={}",
                         i, c, instance.position.x, instance.position.y, instance.position.z, instance.size.x,
                         instance.size.y, instance.glyph_index, instance.sdf_params.x, instance.sdf_params.y,
                         instance.sdf_params.z, instance.sdf_params.w, instance.texture_index);

        text_instances.push_back(instance);
        x += glyph.advance * scale;
    }

    ENGINE_LOG_DEBUG("Added {} instances, total instances: {}", text_instances.size() - instance_count,
                     text_instances.size());
}

void Renderer::SetupPipelines(std::string_view quad_vertex_shader_path, std::string_view quad_fragment_shader_path,
                              std::string_view text_vertex_shader_path, std::string_view text_fragment_shader_path,
                              std::string_view particle_vertex_shader_path,
                              std::string_view particle_fragment_shader_path,
                              std::string_view particle_compute_shader_path)
{
    p_quad_vertex_shader = std::make_unique<Shader>(*p_device, quad_vertex_shader_path);
    p_quad_fragment_shader = std::make_unique<Shader>(*p_device, quad_fragment_shader_path);
    p_quad_pipeline = std::make_unique<GraphicsPipeline>(*this, p_window, p_render_pass, p_quad_vertex_shader.get(),
                                                         p_quad_fragment_shader.get(), p_swapchain->GetImageCount(),
                                                         m_uniform_buffers, sizeof(UniformData), PipelineType::Quad);

    p_text_vertex_shader = std::make_unique<Shader>(*p_device, text_vertex_shader_path);
    p_text_fragment_shader = std::make_unique<Shader>(*p_device, text_fragment_shader_path);
    p_text_pipeline = std::make_unique<GraphicsPipeline>(*this, p_window, p_render_pass, p_text_vertex_shader.get(),
                                                         p_text_fragment_shader.get(), p_swapchain->GetImageCount(),
                                                         m_uniform_buffers, sizeof(UniformData), PipelineType::Text);

    p_particle_vertex_shader = std::make_unique<Shader>(*p_device, particle_vertex_shader_path);
    p_particle_fragment_shader = std::make_unique<Shader>(*p_device, particle_fragment_shader_path);
    p_particle_pipeline = std::make_unique<GraphicsPipeline>(
        *this, p_window, p_render_pass, p_particle_vertex_shader.get(), p_particle_fragment_shader.get(),
        p_swapchain->GetImageCount(), m_uniform_buffers, sizeof(UniformData), PipelineType::Particle);

    p_particle_compute_shader = std::make_unique<Shader>(*p_device, particle_compute_shader_path);
    p_particle_compute_pipeline = std::make_unique<ComputePipeline>(
        *this, p_particle_compute_shader.get(), m_particle_storage_buffers, m_compute_uniform_buffers,
        sizeof(ParticleData) * m_max_particle_instances, sizeof(SimulationParams));
}

void Renderer::CreateFramebuffers()
{
    m_framebuffers.clear();
    m_framebuffers.resize(p_swapchain->GetImages().size());

    FrameBufferSize swapchain_size = p_swapchain->GetFramebufferSize();
    ENGINE_LOG_DEBUG("Framebuffer size from swapchain: {}x{}", swapchain_size.width, swapchain_size.height);

    ASSERT(m_framebuffer_size.area() > 0, "Framebuffer dimensions cannot be zero.");

    for (u32 i = 0; i < p_swapchain->GetImages().size(); i++) {
        std::vector<VkImageView> attachments;
        attachments.emplace_back(p_swapchain->GetImageViews()[i]);
        attachments.emplace_back(p_depth_resources->GetDepthImages()[i].p_view);

        VkFramebufferCreateInfo frame_buffer_create_info{};
        frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buffer_create_info.renderPass = p_render_pass;
        frame_buffer_create_info.attachmentCount = static_cast<u32>(attachments.size());
        frame_buffer_create_info.pAttachments = attachments.data();
        frame_buffer_create_info.width = static_cast<u32>(swapchain_size.width);
        frame_buffer_create_info.height = static_cast<u32>(swapchain_size.height);
        frame_buffer_create_info.layers = 1;

        VkResult result{
            vkCreateFramebuffer(p_device->GetDevice(), &frame_buffer_create_info, nullptr, &m_framebuffers[i])};
        CHECK_VK_RESULT(result, "vkCreateFramebuffer\n");
    }

    ENGINE_LOG_DEBUG("Framebuffers created.");
}

void Renderer::DestroyFramebuffers()
{
    for (auto buffer : m_framebuffers) {
        vkDestroyFramebuffer(p_device->GetDevice(), buffer, nullptr);
    }

    m_framebuffers.clear();

    ENGINE_LOG_DEBUG("Framebuffers destroyed.");
}

void Renderer::CreateCommandBuffers()
{
    m_command_buffers.clear();
    m_command_buffers.resize(p_swapchain->GetImageCount());

    p_command_buffer_manager->AllocateBuffers(static_cast<u32>(m_command_buffers.size()), m_command_buffers.data());
}

void Renderer::CreateUniformBuffers(size_t data_size)
{
    m_uniform_buffers.clear();
    m_uniform_buffers.reserve(p_swapchain->GetImages().size());

    for (size_t i = 0; i < p_swapchain->GetImages().size(); ++i) {
        m_uniform_buffers.emplace_back(p_buffer_manager->CreateUniformBuffer(data_size));
    }
}

u32 Renderer::LoadTexture(std::string_view filepath)
{
    if (m_textures.size() == MAX_TEXTURES) {
        ENGINE_LOG_ERROR("Cannot create and add texture '{}' as array is already full. (Max slots: {})", filepath,
                         MAX_TEXTURES);
        return 0;
    }

    auto texture = p_buffer_manager->CreateTexture(filepath);
    m_textures.push_back(std::move(texture));
    return static_cast<u32>(m_textures.size() - 1);
}

u32 Renderer::LoadMSDFFont(std::string_view image_filepath, std::string_view json_filepath)
{
    u32 font_id{LoadTexture(image_filepath)};
    m_fonts[font_id] = load_msdf_glyphs(json_filepath);
    return font_id;
}

void Renderer::SetClearColour(const Colour<f32> &colour)
{
    m_clear_colour = {{colour.value[0], colour.value[1], colour.value[2], colour.value[3]}};
}

void Renderer::InitializeCore(GLFWwindow *window_ptr, std::string_view app_name, SemVer vulkan_api_version)
{
    p_window = window_ptr;
    CacheFrameBufferSize();

    p_instance = std::make_unique<Instance>(app_name, vulkan_api_version, p_window);
    ENGINE_LOG_DEBUG("VulkanInstance initialized.");

    p_device = std::make_unique<Device>(*p_instance, VK_QUEUE_GRAPHICS_BIT);
    ENGINE_LOG_DEBUG("VulkanDevice initialized.");

    p_buffer_manager = std::make_unique<BufferManager>(p_device.get(), &m_queue);
    ENGINE_LOG_DEBUG("Buffer manager initialized.");
}

void Renderer::InitializeSwapchainAndQueue(VSyncMode vsync_mode)
{
    p_swapchain =
        std::make_unique<Swapchain>(p_window, p_device.get(), p_instance.get(), p_buffer_manager.get(), vsync_mode);
    ENGINE_LOG_DEBUG("Swapchain initialized.");

    m_queue.Initialize(p_device.get(), p_swapchain.get(), p_device->GetQueueFamily(), 0);
    ENGINE_LOG_DEBUG("Queue initialized.");
}

void Renderer::InitializeDefaultResources()
{
    auto default_texture = p_buffer_manager->CreateDefaultTexture();
    m_textures.push_back(std::move(default_texture));
    ENGINE_LOG_DEBUG("Default texture created and added to textures.");

    std::vector<Vertex> quad_vertices{{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                                      {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                                      {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
                                      {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}};
    VkDeviceSize vertex_buffer_size{sizeof(Vertex) * quad_vertices.size()};
    p_quad_vertex_buffer =
        std::make_unique<Buffer>(p_buffer_manager->CreateVertexBuffer(quad_vertices.data(), vertex_buffer_size));

    std::vector<u32> quad_indices{0, 1, 2, 0, 2, 3};
    VkDeviceSize index_buffer_size{sizeof(u32) * quad_indices.size()};
    p_quad_index_buffer =
        std::make_unique<Buffer>(p_buffer_manager->CreateIndexBuffer(quad_indices.data(), index_buffer_size));

    m_vertex_count = static_cast<u32>(quad_vertices.size());
    m_index_count = static_cast<u32>(quad_indices.size());
}

void Renderer::InitializeRenderResources()
{
    CreateFences();
    CreateInstanceBuffers();

    p_command_buffer_manager =
        std::make_unique<CommandBufferManager>(p_device.get(), &m_queue, p_device->GetQueueFamily());

    p_command_buffer_manager->AllocateBuffers(1, &p_copy_command_buffer);
    CreateCommandBuffers();

    p_depth_resources =
        std::make_unique<DepthResources>(p_device.get(), p_instance.get(), p_buffer_manager.get(), p_swapchain.get());

    p_render_pass = CreateRenderPass();
    CreateFramebuffers();

    SetClearColour({0.0f, 0.0f, 0.0f, 0.0f});
}

VkRenderPass Renderer::CreateRenderPass()
{
    ENGINE_PROFILE_SCOPE("Create render passes");

    VkAttachmentDescription colour_attachment_description{};
    colour_attachment_description.flags = 0;
    colour_attachment_description.format = p_swapchain->GetSurfaceFormat().format;
    colour_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    colour_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colour_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colour_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colour_attachment_reference{.attachment = 0,
                                                      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkFormat depth_format{p_device->GetSelectedPhysicalDevice().m_depth_format};

    VkAttachmentDescription depth_attachment_description{};
    depth_attachment_description.format = depth_format;
    depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference{.attachment = 1,
                                                     .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    std::vector<VkSubpassDependency> subpass_dependencies{
        {.srcSubpass = VK_SUBPASS_EXTERNAL,
         .dstSubpass = 0,
         .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
         .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
         .srcAccessMask = 0,
         .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
         .dependencyFlags = 0}};

    VkSubpassDescription subpass_description{};
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &colour_attachment_reference;
    subpass_description.pResolveAttachments = nullptr;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = nullptr;

    std::vector<VkAttachmentDescription> attachment_descriptions;
    attachment_descriptions.push_back(colour_attachment_description);
    attachment_descriptions.push_back(depth_attachment_description);

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = nullptr;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = static_cast<u32>(attachment_descriptions.size());
    render_pass_create_info.pAttachments = attachment_descriptions.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = static_cast<u32>(subpass_dependencies.size());
    render_pass_create_info.pDependencies = subpass_dependencies.data();

    VkRenderPass render_pass{VK_NULL_HANDLE};
    VkResult result{vkCreateRenderPass(p_device->GetDevice(), &render_pass_create_info, nullptr, &render_pass)};
    CHECK_VK_RESULT(result, "vkCreateRenderPass\n");

    ENGINE_LOG_DEBUG("Render pass created.");

    return render_pass;
}

void Renderer::CreateFences()
{
    const auto max_frames = m_queue.GetMaxFramesInFlight();
    VkFenceCreateFlags fence_flags{VK_FENCE_CREATE_SIGNALED_BIT};

    for (u32 i = 0; i < max_frames; i++) {
        m_frame_fences.emplace_back(p_device.get()); // Construct the fence and pass the device
        if (!m_frame_fences[i].Create(fence_flags)) {
            ENGINE_THROW("Could not create frame fence {} in renderer.", i);
        }
    }

    ENGINE_LOG_DEBUG("Frame fences created.");
}

void Renderer::CacheFrameBufferSize()
{
    glfwGetWindowSize(p_window, &m_framebuffer_size.width, &m_framebuffer_size.height);
}

void Renderer::ReCreateSwapchain()
{
    DestroyFramebuffers();

    p_swapchain->Recreate();
    m_queue.SetSwapchain(p_swapchain.get());
    CacheFrameBufferSize();
    p_depth_resources->Recreate();

    CreateFramebuffers();
}

void Renderer::CreateInstanceBuffers()
{
    VkDeviceSize max_quad_instance_size{sizeof(InstanceData) * m_max_quad_instances};
    VkDeviceSize max_text_instance_size{sizeof(TextData) * m_max_text_instances};
    VkDeviceSize max_particle_instance_size{sizeof(ParticleData) * m_max_particle_instances};

    m_quad_instance_buffers.resize(p_swapchain->GetImageCount());
    m_text_instance_buffers.resize(p_swapchain->GetImageCount());
    m_particle_instance_buffers.resize(p_swapchain->GetImageCount());
    m_particle_storage_buffers.resize(p_swapchain->GetImageCount());
    m_compute_uniform_buffers.resize(p_swapchain->GetImageCount());

    m_mapped_quad_instance_data.resize(p_swapchain->GetImageCount());
    m_mapped_text_instance_data.resize(p_swapchain->GetImageCount());
    m_mapped_particle_instance_data.resize(p_swapchain->GetImageCount());
    m_mapped_particle_storage_data.resize(p_swapchain->GetImageCount());

    for (size_t i = 0; i < p_swapchain->GetImageCount(); ++i) {
        m_quad_instance_buffers[i] = p_buffer_manager->CreateDynamicVertexBuffer(max_quad_instance_size);
        m_mapped_quad_instance_data[i] = m_quad_instance_buffers[i].MapPersistent(p_device->GetDevice());

        m_text_instance_buffers[i] = p_buffer_manager->CreateDynamicVertexBuffer(max_text_instance_size);
        m_mapped_text_instance_data[i] = m_text_instance_buffers[i].MapPersistent(p_device->GetDevice());

        m_particle_instance_buffers[i] = p_buffer_manager->CreateDynamicVertexBuffer(max_particle_instance_size);
        m_mapped_particle_instance_data[i] = m_particle_instance_buffers[i].MapPersistent(p_device->GetDevice());

        m_particle_storage_buffers[i] = p_buffer_manager->CreateStorageBuffer(max_particle_instance_size);
        m_mapped_particle_storage_data[i] = m_particle_storage_buffers[i].MapPersistent(p_device->GetDevice());

        m_compute_uniform_buffers[i] = p_buffer_manager->CreateUniformBuffer(sizeof(SimulationParams));
    }
}

void Renderer::InitializeImGUIIfEnabled()
{
#ifdef USE_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io{ImGui::GetIO()};
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = "config/imgui_settings.ini";
    io.LogFilename = "debug/logs/imgui_log.txt";

    ImGui::StyleColorsDark();
    ImGuiStyle &style{ImGui::GetStyle()};
    style.WindowRounding = 5.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

    ImGui_ImplGlfw_InitForVulkan(p_window, false);

    VkDescriptorPoolSize pool_sizes[]{{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = static_cast<u32>(IM_ARRAYSIZE(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(p_device->GetDevice(), &pool_info, nullptr, &p_imgui_pool) != VK_SUCCESS) {
        ENGINE_THROW("Failed to create ImGui descriptor pool!");
    }

    VkQueue graphics_queue{VK_NULL_HANDLE};
    vkGetDeviceQueue(GetDevice(), p_device->GetQueueFamily(), 0, &graphics_queue);

    u32 min_image_count{p_device->GetSelectedPhysicalDevice().m_surface_capabilities.minImageCount};

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = p_instance->GetInstance();
    init_info.PhysicalDevice = p_device->GetPhysicalDevice();
    init_info.Device = p_device->GetDevice();
    init_info.QueueFamily = p_device->GetQueueFamily();
    init_info.Queue = graphics_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = p_imgui_pool;
    init_info.RenderPass = p_render_pass;
    init_info.Allocator = nullptr;
    init_info.MinImageCount = min_image_count;
    init_info.ImageCount = p_swapchain->GetImageCount();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    ENGINE_LOG_DEBUG("ImGUI initialized.");
#endif
}

ImDrawData *Renderer::RenderImGUI(const RenderStatistics &stats)
{
#ifdef USE_IMGUI
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static int location = 0;
    static bool open{true};
    ImGuiIO &io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                    ImGuiWindowFlags_NoNav;
    if (location >= 0) {
        const float PAD = 10.0f;
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    else if (location == -2) {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    ImGui::SetNextWindowBgAlpha(0.35f);
    if (ImGui::Begin("Stats for nerds", &open, window_flags)) {
        ImGui::Text("Stats for nerds.");
        ImGui::Separator();
        ImGui::Text("Deltatime: %f", stats.delta_time);
        ImGui::Text("FPS: %f", (stats.delta_time > 0.0f) ? (1.0f / stats.delta_time) : 0.0f);
        ImGui::Separator();
        ImGui::Text("Quads: %u / %u", stats.instance_count, m_max_quad_instances);
        ImGui::Text("Vertices: %u (per instance: %u)", stats.vertex_count * stats.instance_count, stats.vertex_count);
        ImGui::Text("Indices: %u (per instance: %u)", stats.index_count * stats.instance_count, stats.index_count);
        ImGui::Text("Particles: %u", stats.particle_count);
        ImGui::Text("Glyphs: %u", stats.glyph_count);
        ImGui::Text("Textures: %u", stats.texture_count);
        ImGui::Separator();
        ImGui::Text("Compute: %u", m_use_compute_particles);
        if (ImGui::IsMousePosValid())
            ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
        else
            ImGui::Text("Mouse Position: <invalid>");
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Custom", NULL, location == -1))
                location = -1;
            if (ImGui::MenuItem("Center", NULL, location == -2))
                location = -2;
            if (ImGui::MenuItem("Top-left", NULL, location == 0))
                location = 0;
            if (ImGui::MenuItem("Top-right", NULL, location == 1))
                location = 1;
            if (ImGui::MenuItem("Bottom-left", NULL, location == 2))
                location = 2;
            if (ImGui::MenuItem("Bottom-right", NULL, location == 3))
                location = 3;
            if (open && ImGui::MenuItem("Close"))
                open = false;
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    return draw_data;
#else
    return nullptr;
#endif
}

void Renderer::DestroyImGUI()
{
#ifdef USE_IMGUI
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(p_device->GetDevice(), p_imgui_pool, nullptr);

    ENGINE_LOG_DEBUG("ImGUI destroyed.");
#endif
}

void Renderer::DestroyBuffers()
{
    for (auto &buffer : m_uniform_buffers) {
        buffer.Destroy(p_device->GetDevice());
    }
    ENGINE_LOG_DEBUG("Uniform buffers destroyed.");

    for (auto &buffer : m_compute_uniform_buffers) {
        buffer.Destroy(p_device->GetDevice());
    }
    ENGINE_LOG_DEBUG("Compute uniform buffers destroyed.");

    for (auto &buffer : m_quad_instance_buffers) {
        buffer.Destroy(p_device->GetDevice());
    }
    ENGINE_LOG_DEBUG("Quad instance buffers destroyed.");

    for (auto &buffer : m_text_instance_buffers) {
        buffer.Destroy(p_device->GetDevice());
    }
    ENGINE_LOG_DEBUG("Text instance buffers destroyed.");

    for (auto &buffer : m_particle_instance_buffers) {
        buffer.Destroy(p_device->GetDevice());
    }
    ENGINE_LOG_DEBUG("Particle instance buffers destroyed.");

    for (auto &buffer : m_particle_storage_buffers) {
        buffer.Destroy(p_device->GetDevice());
    }
    ENGINE_LOG_DEBUG("Particle storage buffers destroyed.");

    if (p_quad_vertex_buffer != nullptr) {
        p_quad_vertex_buffer->Destroy(p_device->GetDevice());
        ENGINE_LOG_DEBUG("Quad vertex buffer destroyed.");
    }

    if (p_quad_index_buffer != nullptr) {
        p_quad_index_buffer->Destroy(p_device->GetDevice());
        ENGINE_LOG_DEBUG("Quad index buffer destroyed.");
    }
}

} // namespace gouda::vk