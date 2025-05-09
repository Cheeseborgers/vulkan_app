#pragma once
/**
 * @file renderers/vulkan/vk_renderer.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-25
 * @brief Engine Vulkan renderer module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <string_view>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui.h"

#include "math/math.hpp"
#include "renderers/render_data.hpp"
#include "renderers/text.hpp"
#include "renderers/vulkan/vk_buffer.hpp"
#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_queue.hpp"
#include "renderers/vulkan/vk_swapchain.hpp"

namespace gouda::vk {

class Device;
class Fence;
class Texture;
class Instance;
class Shader;
class DepthResources;
class GraphicsPipeline;
class ComputePipeline;
class CommandBufferManager;

class Renderer {
public:
    Renderer();
    ~Renderer();

    void Initialize(GLFWwindow *window_ptr, std::string_view app_name, SemVer vulkan_api_version = {1, 3, 0, 0},
                    VSyncMode vsync_mode = VSyncMode::Enabled);

    void RecordCommandBuffer(VkCommandBuffer command_buffer, u32 image_index, u32 quad_instance_count,
                             u32 text_instance_count, u32 particle_instance_count, ImDrawData *draw_data);

    void Render(f32 delta_time, const UniformData &uniform_data, const std::vector<InstanceData> &quad_instances,
                const std::vector<TextData> &text_instances, const std::vector<ParticleData> &particle_instances);

    void UpdateComputeUniformBuffer(u32 image_index, f32 delta_time);
    void UpdateParticleStorageBuffer(u32 image_index, const std::vector<ParticleData> &particle_instances);
    void ClearParticleBuffers(u32 image_index);

    void ToggleComputeParticles();
    bool UseComputeParticles() const { return m_use_compute_particles; }

    void DrawText(std::string_view text, Vec2 position, f32 scale, u32 font_id, std::vector<TextData> &text_instances);

    void SetupPipelines(std::string_view quad_vertex_shader_path, std::string_view quad_fragment_shader_path,
                        std::string_view text_vertex_shader_path, std::string_view text_fragment_shader_path,
                        std::string_view particle_vertex_shader_path, std::string_view particle_fragment_shader_path,
                        std::string_view particle_compute_shader_path);

    void CreateFramebuffers();
    void DestroyFramebuffers();
    void CreateCommandBuffers();
    void CreateUniformBuffers(size_t data_size);

    BufferManager *GetBufferManager() { return p_buffer_manager.get(); }
    FrameBufferSize GetFramebufferSize() const { return m_framebuffer_size; }
    VkDevice GetDevice() { return p_device->GetDevice(); }
    Buffer *GetStaticVertexBuffer() { return p_quad_vertex_buffer.get(); }
    const std::vector<Buffer> &GetInstanceBuffers() { return m_quad_instance_buffers; }
    const std::vector<std::unique_ptr<Texture>> &GetTextures() { return m_textures; }

    u32 LoadTexture(std::string_view filepath);
    u32 LoadMSDFFont(std::string_view image_filepath, std::string_view json_filepath);
    void SetClearColour(const Colour<f32> &colour);
    void ReCreateSwapchain();
    void DeviceWait() { p_device->Wait(); }

private:
    struct RenderStatistics {
        f32 delta_time;
        u32 instance_count;
        u32 vertex_count;
        u32 index_count;
        u32 particle_count;
        u32 glyph_count;
        u32 texture_count;
    };

private:
    void InitializeCore(GLFWwindow *window_ptr, std::string_view app_name, SemVer vulkan_api_version);
    void InitializeSwapchainAndQueue(VSyncMode vsync_mode);
    void InitializeDefaultResources();
    void InitializeRenderResources();
    VkRenderPass CreateRenderPass();
    void CreateFences();
    void CacheFrameBufferSize();
    void CreateInstanceBuffers();
    void InitializeImGUIIfEnabled();
    ImDrawData *RenderImGUI(const RenderStatistics &stats);
    void UpdateTextureDescriptors();
    void DestroyImGUI();
    void DestroyBuffers();

private:
    std::unique_ptr<Instance> p_instance;
    std::unique_ptr<Device> p_device;
    std::unique_ptr<BufferManager> p_buffer_manager;
    std::unique_ptr<Swapchain> p_swapchain;
    std::unique_ptr<DepthResources> p_depth_resources;
    std::unique_ptr<CommandBufferManager> p_command_buffer_manager;

    std::unique_ptr<GraphicsPipeline> p_quad_pipeline;
    std::unique_ptr<GraphicsPipeline> p_text_pipeline;
    std::unique_ptr<GraphicsPipeline> p_particle_pipeline;
    std::unique_ptr<ComputePipeline> p_particle_compute_pipeline;

    std::unique_ptr<Buffer> p_quad_vertex_buffer;
    std::unique_ptr<Buffer> p_quad_index_buffer;

    std::unique_ptr<Shader> p_quad_vertex_shader;
    std::unique_ptr<Shader> p_quad_fragment_shader;
    std::unique_ptr<Shader> p_text_vertex_shader;
    std::unique_ptr<Shader> p_text_fragment_shader;
    std::unique_ptr<Shader> p_particle_vertex_shader;
    std::unique_ptr<Shader> p_particle_fragment_shader;
    std::unique_ptr<Shader> p_particle_compute_shader;

    GLFWwindow *p_window;
    VkRenderPass p_render_pass;
    VkCommandBuffer p_copy_command_buffer;
    VkDescriptorPool p_imgui_pool;
    Queue m_queue;

    std::vector<Fence> m_frame_fences;
    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<VkCommandBuffer> m_command_buffers;

    std::vector<Buffer> m_uniform_buffers;
    std::vector<Buffer> m_compute_uniform_buffers;
    std::vector<Buffer> m_quad_instance_buffers;
    std::vector<Buffer> m_text_instance_buffers;
    std::vector<Buffer> m_particle_instance_buffers;
    std::vector<Buffer> m_particle_storage_buffers;

    std::vector<void *> m_mapped_quad_instance_data;
    std::vector<void *> m_mapped_text_instance_data;
    std::vector<void *> m_mapped_particle_instance_data;
    std::vector<void *> m_mapped_particle_storage_data;

    std::vector<ParticleData> m_particles_instances;

    std::vector<std::unique_ptr<Texture>> m_textures;
    std::unordered_map<u32, std::unordered_map<char, Glyph>> m_fonts;

    FrameBufferSize m_framebuffer_size;
    u32 m_current_frame;

    SimulationParams m_simulation_params;

    VkClearColorValue m_clear_colour;
    size_t m_max_quad_instances;
    size_t m_max_text_instances;
    size_t m_max_particle_instances;
    VSyncMode m_vsync_mode;
    u32 m_vertex_count;
    u32 m_index_count;
    bool m_is_initialized;
    bool m_use_compute_particles;
    bool m_textures_dirty;
};

} // namespace gouda::vk