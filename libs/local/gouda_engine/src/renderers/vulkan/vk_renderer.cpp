/**
 * @file vk_renderer.cpp
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

// TODO: Remove this dependency
#include "renderers/vulkan/gouda_vk_wrapper.hpp"

namespace gouda::vk {

Renderer::Renderer()
    : p_instance{nullptr},
      p_device{nullptr},
      p_buffer_manager{nullptr},
      p_swapchain{nullptr},
      p_depth_resources{nullptr},
      p_command_buffer_manager{nullptr},
      p_texture_manager{nullptr},
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
      m_framebuffer_size{0, 0},
      m_current_frame{0},
      m_simulation_params{{0.0f, constants::gravity, 0.0f}, 0.0f},
      m_clear_colour{},
      m_max_quad_instances{1000},
      m_max_text_instances{1000},
      m_max_particle_instances{1024}, // Multiple of 256 for compute
      m_vsync_mode{VSyncMode::Disabled},
      m_vertex_count{0},
      m_index_count{0},
      m_is_initialized{false},
      m_use_compute_particles{false},
      m_font_textures_dirty{true}
{
}

Renderer::~Renderer()
{
    ENGINE_LOG_DEBUG("Cleaning up Vulkan renderer.");

    if (m_is_initialized) {
        DestroyBuffers();

        for (const auto &texture : m_font_textures) {
            texture->Destroy(p_device.get());
        }

        if (p_render_pass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(p_device->GetDevice(), p_render_pass, nullptr);
            p_render_pass = VK_NULL_HANDLE;
            ENGINE_LOG_DEBUG("Render pass destroyed.");
        }

        DestroyFramebuffers();
        DestroyImGUI();

        // Destroy in reverse order to ensure dependencies are cleaned up properly
        p_buffer_manager.reset();
        p_command_buffer_manager.reset();
    }
}

void Renderer::Initialize(GLFWwindow *window_ptr, StringView app_name, const SemVer vulkan_api_version,
                          const VSyncMode vsync_mode)
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

void Renderer::RecordCommandBuffer(VkCommandBuffer command_buffer, const u32 image_index, const u32 quad_instance_count,
                                   const u32 text_instance_count, const u32 particle_instance_count,
                                   ImDrawData *draw_data) const
{
    ENGINE_PROFILE_SCOPE("Record command buffer");

    BeginCommandBuffer(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // Update Particles ---------------------------------------------
    if (m_use_compute_particles && particle_instance_count > 0) {
        // ENGINE_LOG_DEBUG("Compute dispatch: particle_count = {}, workgroup_count = [{}, {}, {}]",
        //          particle_instance_count, (particle_instance_count + 255) / 256, 1, 1);
        p_particle_compute_pipeline->Bind(command_buffer);
        p_particle_compute_pipeline->BindDescriptors(command_buffer, image_index);
        const u32 particle_count{math::min(particle_instance_count, m_max_particle_instances)};
        const UVec3 workgroup_count{(particle_count + 255) / 256, 1, 1};
        p_particle_compute_pipeline->Dispatch(command_buffer, workgroup_count);

        // Barrier: Compute shader write to vertex attribute read
        const VkBufferMemoryBarrier barrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = m_particle_storage_buffers[image_index].p_buffer,
            .offset = 0,
            .size = sizeof(ParticleData) * particle_count,
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                             0, 0, nullptr, 1, &barrier, 0, nullptr);
    }

    // Begin Render pass
    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = m_clear_colour;
    clear_values[1].depthStencil = {1.0f, 0};

    const VkExtent2D extent{p_swapchain->GetExtent()};
    const VkRenderPassBeginInfo render_pass_begin_info{.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                                       .renderPass = p_render_pass,
                                                       .framebuffer = m_framebuffers[image_index],
                                                       .renderArea = {{0, 0}, extent},
                                                       .clearValueCount = static_cast<u32>(clear_values.size()),
                                                       .pClearValues = clear_values.data()};

    const VkViewport viewport{.x = 0.0f,
                              .y = 0.0f,
                              .width = static_cast<f32>(extent.width),
                              .height = static_cast<f32>(extent.height),
                              .minDepth = 0.0f,
                              .maxDepth = 1.0f};

    const VkRect2D scissor{{0, 0}, extent};

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    // Quad/Sprite rendering
    if (quad_instance_count > 0) {
        p_quad_pipeline->Bind(command_buffer, image_index);
        const VkBuffer buffers[]{p_quad_vertex_buffer->p_buffer, m_quad_instance_buffers[image_index].p_buffer};
        constexpr VkDeviceSize offsets[]{0, 0};
        vkCmdBindVertexBuffers(command_buffer, 0, 2, buffers, offsets);
        vkCmdBindIndexBuffer(command_buffer, p_quad_index_buffer->p_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer, m_index_count, quad_instance_count, 0, 0, 0);
    }

    // Text rendering
    if (text_instance_count > 0 && p_text_pipeline) {
        p_text_pipeline->Bind(command_buffer, image_index);
        const VkBuffer buffers[]{p_quad_vertex_buffer->p_buffer, m_text_instance_buffers[image_index].p_buffer};
        constexpr VkDeviceSize offsets[]{0, 0};
        vkCmdBindVertexBuffers(command_buffer, 0, 2, buffers, offsets);
        vkCmdBindIndexBuffer(command_buffer, p_quad_index_buffer->p_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer, m_index_count, text_instance_count, 0, 0, 0);
    }

    // Particle rendering
    if (p_particle_pipeline && particle_instance_count > 0) {
        // ENGINE_LOG_DEBUG("Particle draw: instance_count = {}", particle_instance_count);
        p_particle_pipeline->Bind(command_buffer, image_index);
        const VkBuffer buffers[]{p_quad_vertex_buffer->p_buffer, m_particle_storage_buffers[image_index].p_buffer};
        constexpr VkDeviceSize offsets[]{0, 0};
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

void Renderer::Render(const f32 delta_time, const UniformData &uniform_data,
                      const std::vector<InstanceData> &quad_instances, const std::vector<TextData> &text_instances,
                      const std::vector<ParticleData> &particle_instances)
{
    while (!p_swapchain->IsValid()) {
        std::this_thread::yield();
        ENGINE_LOG_INFO("Rendering paused, waiting for valid swapchain");
    }

    UpdateTextureDescriptors();

    m_frame_fences[m_current_frame].WaitFor(constants::u64_max);
    m_frame_fences[m_current_frame].Reset();

    const u32 image_index{m_queue.AcquireNextImage(m_current_frame)};
    if (image_index == constants::u32_max) {
        return;
    }

    // Update compute uniform buffer
    UpdateComputeUniformBuffer(image_index, delta_time);
    UpdateParticleStorageBuffer(image_index, particle_instances);

    // Update particle instances
    if (m_use_compute_particles && !particle_instances.empty()) {
        // ENGINE_LOG_DEBUG("Compute particles enabled");
        const ParticleData *storage_particles =
            static_cast<ParticleData *>(m_mapped_particle_storage_data[image_index]);
        m_particles_instances.clear();
        m_particles_instances.reserve(particle_instances.size());
        for (size_t i = 0; i < particle_instances.size(); ++i) {
            if (storage_particles[i].lifetime > 0.0f && storage_particles[i].size.x > 0.0f &&
                storage_particles[i].size.y > 0.0f) {
                m_particles_instances.push_back(storage_particles[i]);
            }
        }
    }
    else {
        // CPU path: Use m_particles_instances directly
        m_particles_instances = particle_instances; // Already updated in Scene::UpdateParticles
    }

    // Update uniform buffer
    m_uniform_buffers[image_index].Update(p_device->GetDevice(), &uniform_data, sizeof(uniform_data));

    // Update quad instance data
    const VkDeviceSize quad_instance_size{sizeof(InstanceData) * quad_instances.size()};
    ASSERT(quad_instance_size <= sizeof(InstanceData) * m_max_quad_instances,
           "Quad instance count exceeds maximum buffer size.");
    if (quad_instance_size > 0) {
        memcpy(m_mapped_quad_instance_data[image_index], quad_instances.data(), quad_instance_size);
    }

    // Update text instance data
    const VkDeviceSize text_instance_size{sizeof(TextData) * text_instances.size()};
    ASSERT(text_instance_size <= sizeof(TextData) * m_max_text_instances,
           "Text instance count exceeds maximum buffer size.");
    if (text_instance_size > 0) {
        memcpy(m_mapped_text_instance_data[image_index], text_instances.data(), text_instance_size);
    }

    // Render ImGui
    ImDrawData *imgui_draw_data{nullptr};
#ifdef USE_IMGUI
    const RenderStatistics stats{
        .delta_time = delta_time,
        .instance_count = static_cast<u32>(quad_instances.size()),
        .vertex_count = m_vertex_count,
        .index_count = m_index_count,
        .particle_count = static_cast<u32>(m_particles_instances.size()),
        .glyph_count = static_cast<u32>(text_instances.size()),
        .texture_count = p_texture_manager->GetTextureCount(),
        .font_count = static_cast<u32>(m_fonts.size()),
    };
    imgui_draw_data = RenderImGUI(stats);
#endif

    vkResetCommandBuffer(m_command_buffers[image_index], 0);
    RecordCommandBuffer(m_command_buffers[image_index], image_index, static_cast<u32>(quad_instances.size()),
                        static_cast<u32>(text_instances.size()), static_cast<u32>(m_particles_instances.size()),
                        imgui_draw_data);

    m_queue.Submit(m_command_buffers[image_index], m_current_frame, &m_frame_fences[m_current_frame]);
    m_queue.Present(image_index, m_current_frame);

    m_current_frame = (m_current_frame + 1) % p_swapchain->GetImageCount();
}

void Renderer::UpdateComputeUniformBuffer(const u32 image_index, const f32 delta_time)
{
    m_simulation_params.delta_time = delta_time;
    m_compute_uniform_buffers[image_index].Update(p_device->GetDevice(), &m_simulation_params,
                                                  sizeof(SimulationParams));
}

void Renderer::UpdateParticleStorageBuffer(const u32 image_index,
                                           const std::vector<ParticleData> &particle_instances) const
{
    const VkDeviceSize particle_instance_size =
        sizeof(ParticleData) * std::min(particle_instances.size(), static_cast<size_t>(m_max_particle_instances));

    // Only update if there are particles
    if (!particle_instances.empty()) {
        memcpy(m_mapped_particle_storage_data[image_index], particle_instances.data(), particle_instance_size);
    }

    // ENGINE_LOG_DEBUG("Updating particle storage buffer[{}]: {} particles, size = {} bytes",
    //           image_index, particle_instances.size(), particle_instance_size);
}

void Renderer::ClearParticleBuffers(const u32 image_index) const
{
    const VkDeviceSize max_particle_instance_size{sizeof(ParticleData) * m_max_particle_instances};
    memset(m_mapped_particle_storage_data[image_index], 0, max_particle_instance_size);
}

void Renderer::ToggleComputeParticles()
{
    m_use_compute_particles = !m_use_compute_particles;
    ENGINE_LOG_DEBUG("Compute particles {}.", m_use_compute_particles ? "enabled" : "disabled");

    // Sync storage buffers with current particle instances
    for (size_t i = 0; i < p_swapchain->GetImageCount(); ++i) {
        UpdateParticleStorageBuffer(i, m_particles_instances);
    }
}

void Renderer::DrawText(StringView text, const Vec3 &position, const Colour<f32> &colour, const f32 scale,
                        const u32 font_id, std::vector<TextData> &text_instances, const TextAlign alignment)
{
    if (m_font_textures.empty()) {
        ENGINE_LOG_ERROR("No Fonts loaded when trying to render: {}, with font_id: {}.", text, font_id);
        return;
    }

    if (!m_fonts.contains(font_id) || !m_font_atlas_params.contains(font_id)) {
        ENGINE_LOG_ERROR("Font ID {} not found!", font_id);
        return;
    }

    const auto &glyphs = m_fonts[font_id];
    const auto &atlas_params = m_font_atlas_params[font_id];
    f32 overall_width{0.0f};

    // First pass: Calculate overall width
    for (char current_character : text) {
        u32 unicode_char = static_cast<unsigned char>(current_character);
        auto it = glyphs.find(current_character);
        if (it == glyphs.end()) {
            ENGINE_LOG_WARNING("MSDFGlyph '{}' (unicode={}) not found in font {}.", current_character, unicode_char, font_id);
            // Fallback to space glyph's advance
            if (auto space_it = glyphs.find(32); space_it != glyphs.end()) {
                overall_width += space_it->second.advance * scale;
            }
            continue;
        }

        const MSDFGlyph &glyph = it->second;
        overall_width += glyph.advance * scale;
    }

    // Adjust starting position based on alignment
    Vec3 current_position = position;
    if (alignment == TextAlign::Center) {
        current_position.x -= overall_width * 0.5f;
    } else if (alignment == TextAlign::Right) {
        current_position.x -= overall_width;
    }
    // Left alignment: no adjustment needed

    // Second pass: Generate TextData instances
    for (char current_character : text) {
        auto unicode_char = static_cast<unsigned char>(current_character);
        auto it = glyphs.find(current_character);
        if (it == glyphs.end()) {
            // Handle missing glyph (e.g., use space or skip)
            ENGINE_LOG_WARNING("MSDFGlyph '{}' (unicode={}) not found in font {}.", current_character, unicode_char, font_id);
            continue;
        }

        const MSDFGlyph &glyph = it->second;

        // Skip glyphs with empty plane bounds or current character is a space (32)
        if (glyph.plane_bounds.IsZero() || unicode_char == 32) {
            current_position.x += glyph.advance * scale;
            continue;
        }

        const Rect plane_bounds{glyph.plane_bounds};
        const Rect atlas_bounds{glyph.atlas_bounds};
        const Vec3 glyph_position{current_position.x + plane_bounds.left * scale,
                                  current_position.y + plane_bounds.bottom * scale, position.z};
        const Vec2 size{(plane_bounds.right - plane_bounds.left) * scale,
                        (plane_bounds.top - plane_bounds.bottom) * scale};

        TextData instance{};
        instance.position = glyph_position;
        instance.size = size;
        instance.colour = colour;
        instance.glyph_index = static_cast<u32>(unicode_char);
        instance.sdf_params = UVRect{atlas_bounds.left, atlas_bounds.bottom, atlas_bounds.right, atlas_bounds.top};
        instance.texture_index = font_id;
        instance.atlas_size = atlas_params.atlas_size;
        instance.px_range = atlas_params.distance_range;

        text_instances.push_back(instance);
        current_position.x += glyph.advance * scale;
    }
}

void Renderer::SetupPipelines(StringView quad_vertex_shader_path, StringView quad_fragment_shader_path,
                              StringView text_vertex_shader_path, StringView text_fragment_shader_path,
                              StringView particle_vertex_shader_path, StringView particle_fragment_shader_path,
                              StringView particle_compute_shader_path)
{
    p_quad_vertex_shader = std::make_unique<Shader>(*p_device, quad_vertex_shader_path);
    p_quad_fragment_shader = std::make_unique<Shader>(*p_device, quad_fragment_shader_path);
    p_quad_pipeline = std::make_unique<GraphicsPipeline>(*this, p_render_pass, p_quad_vertex_shader.get(),
                                                         p_quad_fragment_shader.get(), p_swapchain->GetImageCount(),
                                                         m_uniform_buffers, sizeof(UniformData), PipelineType::Quad);

    p_text_vertex_shader = std::make_unique<Shader>(*p_device, text_vertex_shader_path);
    p_text_fragment_shader = std::make_unique<Shader>(*p_device, text_fragment_shader_path);
    p_text_pipeline = std::make_unique<GraphicsPipeline>(*this, p_render_pass, p_text_vertex_shader.get(),
                                                         p_text_fragment_shader.get(), p_swapchain->GetImageCount(),
                                                         m_uniform_buffers, sizeof(UniformData), PipelineType::Text);

    p_particle_vertex_shader = std::make_unique<Shader>(*p_device, particle_vertex_shader_path);
    p_particle_fragment_shader = std::make_unique<Shader>(*p_device, particle_fragment_shader_path);
    p_particle_pipeline = std::make_unique<GraphicsPipeline>(
        *this, p_render_pass, p_particle_vertex_shader.get(), p_particle_fragment_shader.get(),
        p_swapchain->GetImageCount(), m_uniform_buffers, sizeof(UniformData), PipelineType::Particle);

    p_particle_compute_shader = std::make_unique<Shader>(*p_device, particle_compute_shader_path);
    p_particle_compute_pipeline = std::make_unique<ComputePipeline>(
        *this, p_device.get(), p_particle_compute_shader.get(), m_particle_storage_buffers, m_compute_uniform_buffers,
        sizeof(ParticleData) * m_max_particle_instances, sizeof(SimulationParams));
}

void Renderer::CreateFramebuffers()
{
    m_framebuffers.clear();
    m_framebuffers.resize(p_swapchain->GetImages().size());

    FrameBufferSize swapchain_size = p_swapchain->GetFramebufferSize();
    ENGINE_LOG_DEBUG("Framebuffer size from swapchain: {}x{}", swapchain_size.width, swapchain_size.height);

    ASSERT(m_framebuffer_size.area() > 0, "Framebuffer dimensions cannot be zero.");

    for (size_t i = 0; i < p_swapchain->GetImages().size(); i++) {
        Vector<VkImageView> attachments;
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

        const VkResult result{
            vkCreateFramebuffer(p_device->GetDevice(), &frame_buffer_create_info, nullptr, &m_framebuffers[i])};
        if (result != VK_SUCCESS) {
            CHECK_VK_RESULT(result, "vkCreateFramebuffer");
        }
    }

    ENGINE_LOG_DEBUG("Framebuffers created.");
}

void Renderer::DestroyFramebuffers()
{
    for (const auto buffer : m_framebuffers) {
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

void Renderer::CreateUniformBuffers(const size_t data_size)
{
    m_uniform_buffers.clear();
    m_uniform_buffers.reserve(p_swapchain->GetImages().size());

    for (size_t i = 0; i < p_swapchain->GetImages().size(); ++i) {
        m_uniform_buffers.emplace_back(p_buffer_manager->CreateUniformBuffer(data_size));
    }
}

// Texture functions -----------------------------
u32 Renderer::LoadSingleTexture(StringView filepath) const { return p_texture_manager->LoadSingleTexture(filepath); }

u32 Renderer::LoadAtlasTexture(StringView image_filepath, StringView json_filepath) const
{
    return p_texture_manager->LoadAtlasTexture(image_filepath, json_filepath);
}

const Sprite *Renderer::GetSprite(const u32 texture_id, StringView sprite_name) const
{
    return p_texture_manager->GetSprite(texture_id, sprite_name);
}

const TextureMetadata &Renderer::GetTextureMetadata(const u32 texture_id) const
{
    return p_texture_manager->GetTextureMetadata(texture_id);
}
u32 Renderer::GetTextureCount() const { return p_texture_manager->GetTextureCount(); }

// ------------------------------------------------

u32 Renderer::LoadTexture(StringView filepath, const std::optional<StringView> &json_filepath) const
{
    if (p_texture_manager->GetTextureCount() >= MAX_TEXTURES) {
        ENGINE_LOG_ERROR("Cannot load texture '{}': max textures ({}) reached", filepath, MAX_TEXTURES);
        return 0;
    }

    u32 texture_id{0};
    if (json_filepath) {
        texture_id = p_texture_manager->LoadAtlasTexture(filepath, *json_filepath);
    }
    else {
        texture_id = p_texture_manager->LoadSingleTexture(filepath);
    }

    return texture_id;
}

u32 Renderer::LoadMSDFFont(StringView image_filepath, StringView json_filepath)
{
    if (m_font_textures.size() == MAX_TEXTURES) {
        ENGINE_LOG_ERROR("Cannot load font '{}': max font textures ({}) reached", image_filepath, MAX_TEXTURES);
        return 0;
    }

    const u32 font_id{static_cast<u32>(m_font_textures.size())};
    m_font_textures.push_back(p_buffer_manager->CreateTexture(image_filepath));
    m_fonts[font_id] = load_msdf_glyphs(json_filepath);
    m_font_atlas_params[font_id] = load_msdf_atlas_params(json_filepath);

    //ENGINE_LOG_DEBUG("Atlas params: {}", m_font_atlas_params[font_id].ToString());

    m_font_textures_dirty = true;

    return font_id;
}

void Renderer::SetClearColour(const Colour<f32> &colour) { m_clear_colour = {colour.r, colour.g, colour.b, colour.a}; }

void Renderer::InitializeCore(GLFWwindow *window_ptr, StringView app_name, SemVer vulkan_api_version)
{
    p_window = window_ptr;
    CacheFrameBufferSize();

    p_instance = std::make_unique<Instance>(app_name, vulkan_api_version, p_window);
    ENGINE_LOG_DEBUG("VulkanInstance initialized.");

    p_device = std::make_unique<Device>(*p_instance, VK_QUEUE_GRAPHICS_BIT);
    ENGINE_LOG_DEBUG("VulkanDevice initialized.");

    p_command_buffer_manager =
        std::make_unique<CommandBufferManager>(p_device.get(), &m_queue, p_device->GetQueueFamily());

    p_buffer_manager = std::make_unique<BufferManager>(p_device.get(), &m_queue, p_command_buffer_manager.get());
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
    auto default_font_texture = p_buffer_manager->CreateDefaultTexture();
    m_font_textures.push_back(std::move(default_font_texture));
    ENGINE_LOG_DEBUG("Default font texture created and added to font textures at index 0.");

    const Vector<Vertex> quad_vertices{{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                                       {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                                       {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
                                       {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}};
    const VkDeviceSize vertex_buffer_size{sizeof(Vertex) * quad_vertices.size()};
    p_quad_vertex_buffer =
        std::make_unique<Buffer>(p_buffer_manager->CreateVertexBuffer(quad_vertices.data(), vertex_buffer_size));

    const Vector<u32> quad_indices{0, 1, 2, 0, 2, 3};
    const VkDeviceSize index_buffer_size{sizeof(u32) * quad_indices.size()};
    p_quad_index_buffer =
        std::make_unique<Buffer>(p_buffer_manager->CreateIndexBuffer(quad_indices.data(), index_buffer_size));

    m_vertex_count = static_cast<u32>(quad_vertices.size());
    m_index_count = static_cast<u32>(quad_indices.size());
}

void Renderer::InitializeRenderResources()
{
    CreateFences();
    CreateInstanceBuffers();

    p_command_buffer_manager->AllocateBuffers(1, &p_copy_command_buffer);
    CreateCommandBuffers();

    p_texture_manager = std::make_unique<TextureManager>(p_buffer_manager.get(), p_device.get());

    p_depth_resources =
        std::make_unique<DepthResources>(p_device.get(), p_instance.get(), p_buffer_manager.get(), p_swapchain.get());

    p_render_pass = CreateRenderPass();
    CreateFramebuffers();

    SetClearColour({0.0f, 0.0f, 0.0f, 0.0f});
}

VkRenderPass Renderer::CreateRenderPass() const
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

    constexpr VkAttachmentReference colour_attachment_reference{.attachment = 0,
                                                                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    const VkFormat depth_format{p_device->GetSelectedPhysicalDevice().m_depth_format};

    VkAttachmentDescription depth_attachment_description{};
    depth_attachment_description.format = depth_format;
    depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    constexpr VkAttachmentReference depth_attachment_reference{
        .attachment = 1, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    const Vector<VkSubpassDependency> subpass_dependencies{
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

    SmallVector<VkAttachmentDescription, 2> attachment_descriptions;
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
    if (const VkResult result{
            vkCreateRenderPass(p_device->GetDevice(), &render_pass_create_info, nullptr, &render_pass)};
        result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateRenderPass\n");
    }

    ENGINE_LOG_DEBUG("Render pass created.");

    return render_pass;
}

void Renderer::CreateFences()
{
    constexpr VkFenceCreateFlags fence_flags{VK_FENCE_CREATE_SIGNALED_BIT};

    for (u32 i = 0; i < p_swapchain->GetImageCount(); i++) {
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
    const VkDeviceSize max_quad_instance_size{sizeof(InstanceData) * m_max_quad_instances};
    const VkDeviceSize max_text_instance_size{sizeof(TextData) * m_max_text_instances};
    const VkDeviceSize max_particle_instance_size{sizeof(ParticleData) * m_max_particle_instances};

    m_quad_instance_buffers.resize(p_swapchain->GetImageCount());
    m_text_instance_buffers.resize(p_swapchain->GetImageCount());
    m_particle_storage_buffers.resize(p_swapchain->GetImageCount());
    m_compute_uniform_buffers.resize(p_swapchain->GetImageCount());

    m_mapped_quad_instance_data.resize(p_swapchain->GetImageCount());
    m_mapped_text_instance_data.resize(p_swapchain->GetImageCount());
    m_mapped_particle_storage_data.resize(p_swapchain->GetImageCount());

    for (size_t i = 0; i < p_swapchain->GetImageCount(); ++i) {
        m_quad_instance_buffers[i] = p_buffer_manager->CreateDynamicVertexBuffer(max_quad_instance_size);
        m_mapped_quad_instance_data[i] = m_quad_instance_buffers[i].MapPersistent(p_device->GetDevice());

        m_text_instance_buffers[i] = p_buffer_manager->CreateDynamicVertexBuffer(max_text_instance_size);
        m_mapped_text_instance_data[i] = m_text_instance_buffers[i].MapPersistent(p_device->GetDevice());

        // Create storage buffer with vertex buffer usage
        m_particle_storage_buffers[i] =
            p_buffer_manager->CreateStorageBuffer(max_particle_instance_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
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

    constexpr VkDescriptorPoolSize pool_sizes[]{{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
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

    const u32 min_image_count{p_device->GetSelectedPhysicalDevice().m_surface_capabilities.minImageCount};

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

ImDrawData *Renderer::RenderImGUI(const RenderStatistics &stats) const
{
#ifdef USE_IMGUI
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static int location{0};
    static bool open{true};
    const ImGuiIO &io{ImGui::GetIO()};
    ImGuiWindowFlags window_flags{ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                  ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                  ImGuiWindowFlags_NoNav};
    if (location >= 0) {
        constexpr f32 padding{10.0f};
        const ImGuiViewport *viewport{ImGui::GetMainViewport()};
        const ImVec2 work_pos{viewport->WorkPos};
        const ImVec2 work_size{viewport->WorkSize};
        ImVec2 window_pos;
        ImVec2 window_pos_pivot;
        window_pos.x = (location & 1) ? (work_pos.x + work_size.x - padding) : (work_pos.x + padding);
        window_pos.y = (location & 2) ? (work_pos.y + work_size.y - padding) : (work_pos.y + padding);
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
        ImGui::Text("Delta-time: %f", stats.delta_time);
        ImGui::Text("FPS: %f", stats.delta_time > 0.0f ? 1.0f / stats.delta_time : 0.0f);
        ImGui::Separator();
        ImGui::Text("Quads: %u / %u", stats.instance_count, m_max_quad_instances);
        ImGui::Text("Vertices: %u (per instance: %u)", stats.vertex_count * stats.instance_count, stats.vertex_count);
        ImGui::Text("Indices: %u (per instance: %u)", stats.index_count * stats.instance_count, stats.index_count);
        ImGui::Text("Particles: %u", stats.particle_count);
        ImGui::Text("Glyphs: %u", stats.glyph_count);
        ImGui::Text("Textures: %u", stats.texture_count);
        ImGui::Text("Fonts: %u", stats.font_count);
        ImGui::Text("Compute: %u", m_use_compute_particles);
        ImGui::Separator();

        if (ImGui::IsMousePosValid()) {
            ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
        }
        else {
            ImGui::Text("Mouse Position: <invalid>");
        }

        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Custom", nullptr, location == -1))
                location = -1;
            if (ImGui::MenuItem("Center", nullptr, location == -2))
                location = -2;
            if (ImGui::MenuItem("Top-left", nullptr, location == 0))
                location = 0;
            if (ImGui::MenuItem("Top-right", nullptr, location == 1))
                location = 1;
            if (ImGui::MenuItem("Bottom-left", nullptr, location == 2))
                location = 2;
            if (ImGui::MenuItem("Bottom-right", nullptr, location == 3))
                location = 3;
            if (open && ImGui::MenuItem("Close"))
                open = false;
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    ImGui::Render();
    ImDrawData *draw_data{ImGui::GetDrawData()};
    return draw_data;
#else
    return nullptr;
#endif
}

void Renderer::DestroyImGUI() const
{
#ifdef USE_IMGUI
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(p_device->GetDevice(), p_imgui_pool, nullptr);

    ENGINE_LOG_DEBUG("ImGUI destroyed.");
#endif
}

void Renderer::UpdateTextureDescriptors()
{
    if (p_texture_manager->IsDirty()) {
        p_quad_pipeline->UpdateTextureDescriptors(p_swapchain->GetImageCount(), p_texture_manager->GetTextures());
        p_particle_pipeline->UpdateTextureDescriptors(p_swapchain->GetImageCount(), p_texture_manager->GetTextures());
        p_texture_manager->SetClean();
    }

    if (m_font_textures_dirty) {
        p_text_pipeline->UpdateFontTextureDescriptors(p_swapchain->GetImageCount(), m_font_textures);
        m_font_textures_dirty = false;
    }
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