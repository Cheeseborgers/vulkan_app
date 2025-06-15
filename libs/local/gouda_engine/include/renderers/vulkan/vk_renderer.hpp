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
#include "renderers/vulkan/vk_texture_manager.hpp"

namespace gouda::vk {

class Device;
class Fence;
struct Texture;
class Instance;
class Shader;
class DepthResources;
class GraphicsPipeline;
class ComputePipeline;
class CommandBufferManager;

struct RenderStatistics {
    RenderStatistics();

    f32 delta_time;
    u32 quad_count;
    u32 vertex_count;
    u32 index_count;
    u32 particle_count;
    u32 glyph_count;
    u32 total_instances;
    u32 texture_count;
    u32 font_count;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    void Initialize(GLFWwindow *window_ptr, StringView app_name, SemVer vulkan_api_version = SemVer{1, 4, 0, 0},
                    VSyncMode vsync_mode = VSyncMode::Enabled);

    void RecordCommandBuffer(VkCommandBuffer command_buffer, u32 image_index, u32 quad_instance_count,
                             u32 text_instance_count, u32 particle_instance_count, ImDrawData *draw_data) const;

    void Render(f32 delta_time, const UniformData &uniform_data, const std::vector<InstanceData> &quad_instances,
                const std::vector<TextData> &text_instances, const std::vector<ParticleData> &particle_instances);

    void UpdateComputeUniformBuffer(u32 image_index, f32 delta_time);
    void UpdateParticleStorageBuffer(u32 image_index, const std::vector<ParticleData> &particle_instances) const;
    void ClearParticleBuffers(u32 image_index) const;

    void ToggleComputeParticles();
    bool UseComputeParticles() const { return m_use_compute_particles; }

    void DrawText(StringView text, const Vec3 &position, const Colour<f32> &colour, f32 scale, u32 font_id,
                  std::vector<TextData> &text_instances, TextAlign alignment = TextAlign::Left, bool apply_camera_effects = false);

    void SetupPipelines(StringView quad_vertex_shader_path, StringView quad_fragment_shader_path,
                        StringView text_vertex_shader_path, StringView text_fragment_shader_path,
                        StringView particle_vertex_shader_path, StringView particle_fragment_shader_path,
                        StringView particle_compute_shader_path);

    void CreateFramebuffers();
    void DestroyFramebuffers();
    void CreateCommandBuffers();
    void CreateUniformBuffers(size_t data_size);

    BufferManager *GetBufferManager() const { return p_buffer_manager.get(); }
    FrameBufferSize GetFramebufferSize() const { return m_framebuffer_size; }
    VkDevice GetDevice() const { return p_device->GetDevice(); }
    Buffer *GetStaticVertexBuffer() const { return p_quad_vertex_buffer.get(); }
    const std::vector<Buffer> &GetInstanceBuffers() { return m_quad_instance_buffers; }

    // Texture functions
    u32 LoadTexture(StringView filepath, const std::optional<StringView> &json_filepath = std::nullopt) const;
    u32 LoadSingleTexture(StringView filepath) const;
    u32 LoadAtlasTexture(StringView image_filepath, StringView json_filepath) const;
    const Sprite *GetSprite(u32 texture_id, StringView sprite_name) const;
    const TextureMetadata &GetTextureMetadata(u32 texture_id) const;
    u32 GetTextureCount() const;
    const Vector<std::unique_ptr<Texture>> &GetTextures() const { return p_texture_manager->GetTextures(); }
    TextureManager *GetTextureManager() const { return p_texture_manager.get(); }
    RenderStatistics GetRenderStatistics() const { return m_render_statistics; }

    // Text functions
    u32 LoadMSDFFont(StringView image_filepath, StringView json_filepath);
    const Vector<std::unique_ptr<Texture>> &GetFontTextures() { return m_font_textures; }

    void SetClearColour(const Colour<f32> &colour);
    void ReCreateSwapchain();
    void DeviceWait() const { p_device->Wait(); }


private:
    void InitializeCore(GLFWwindow *window_ptr, StringView app_name, SemVer vulkan_api_version);
    void InitializeSwapchainAndQueue(VSyncMode vsync_mode);
    void InitializeDefaultResources();
    void InitializeRenderResources();
    VkRenderPass CreateRenderPass() const;
    void CreateFences();
    void CacheFrameBufferSize();
    void CreateInstanceBuffers();
    void InitializeImGUIIfEnabled();
    ImDrawData *RenderImGUI() const;
    void UpdateTextureDescriptors();
    void DestroyImGUI() const;
    void DestroyBuffers();

private:
    std::unique_ptr<Instance> p_instance;
    std::unique_ptr<Device> p_device;
    std::unique_ptr<BufferManager> p_buffer_manager;
    std::unique_ptr<Swapchain> p_swapchain;
    std::unique_ptr<DepthResources> p_depth_resources;
    std::unique_ptr<CommandBufferManager> p_command_buffer_manager;
    std::unique_ptr<TextureManager> p_texture_manager;

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
    Vector<VkFramebuffer> m_framebuffers;
    Vector<VkCommandBuffer> m_command_buffers;

    Vector<Buffer> m_uniform_buffers;
    Vector<Buffer> m_compute_uniform_buffers;
    std::vector<Buffer> m_quad_instance_buffers;
    std::vector<Buffer> m_text_instance_buffers;

    std::vector<ParticleData> m_particles_instances;
    Vector<Buffer> m_particle_storage_buffers;
    std::vector<void *> m_mapped_particle_storage_data;

    std::vector<void *> m_mapped_quad_instance_data;
    std::vector<void *> m_mapped_text_instance_data;

    Vector<std::unique_ptr<Texture>> m_font_textures;
    std::unordered_map<u32, MSDFAtlasParams> m_font_atlas_params;
    std::unordered_map<u32, MSDFGlyphMap> m_fonts;

    FrameBufferSize m_framebuffer_size;
    u32 m_current_frame;

    SimulationParams m_simulation_params;
    RenderStatistics m_render_statistics;

    VkClearColorValue m_clear_colour;
    size_t m_max_quad_instances;
    size_t m_max_text_instances;
    u32 m_max_particle_instances;
    VSyncMode m_vsync_mode;
    u32 m_vertex_count;
    u32 m_index_count;
    bool m_is_initialized;
    bool m_use_compute_particles;
    bool m_font_textures_dirty;
};

} // namespace gouda::vk