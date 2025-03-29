#pragma once
/**
 * @file renderers/vulkan/vk_renderer.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-25
 * @brief Engine Vulkan renderer module
 *
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <expected>
#include <string_view>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui.h"             // Core ImGui functionality
#include "imgui_impl_glfw.h"   // GLFW backend (if using GLFW for windowing)
#include "imgui_impl_vulkan.h" // Vulkan backend for ImGui

#include "core/types.hpp"
#include "math/math.hpp"

#include "renderers/vulkan/vk_buffer_manager.hpp"
#include "renderers/vulkan/vk_command_buffer_manager.hpp"
#include "renderers/vulkan/vk_depth_resources.hpp"
#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_graphics_pipeline.hpp"
#include "renderers/vulkan/vk_instance.hpp"
#include "renderers/vulkan/vk_queue.hpp"
#include "renderers/vulkan/vk_swapchain.hpp"
#include "renderers/vulkan/vk_texture.hpp"
#include "renderers/vulkan/vk_uniform_data.hpp"

namespace gouda::vk {

struct Vertex {
    Vertex(const Vec3 &pos, const Vec2 &tex_coords, const Vec4 &col) : position{pos}, uv{tex_coords}, colour{col} {}

    Vec3 position; // 3D position
    Vec2 uv;       // Vertex texture
    Vec4 colour;   // Tint colour
};

struct UniformData {
    Mat4 WVP; // World-View-Projection matrix
};

struct InstanceData {
    InstanceData() : position{0.0f, 0.0f, 0.0f}, scale{1.0f}, rotation{0.0f} {}
    InstanceData(Vec3 position_, f32 scale_, f32 rotation_) : position{position_}, scale{scale_}, rotation{rotation_}
    {
        position.z = math::clamp(position.z, -0.9f, 0.0f);
    }

    Vec3 position; // 3D position
    f32 scale;     // Uniform scale factor
    f32 rotation;  // Rotation in radians
};

class VulkanRenderer {

public:
    VulkanRenderer();
    ~VulkanRenderer();

    void Initialize(GLFWwindow *window_ptr, std::string_view app_name, SemVer vulkan_api_version = {1, 3, 0, 0},
                    VSyncMode vsync_mode = VSyncMode::ENABLED);
    void RecordCommandBuffer(VkCommandBuffer command_buffer, u32 image_index, u32 instance_count,
                             ImDrawData *draw_data);
    void Render(f32 delta_time, const UniformData &uniform_data, const std::vector<InstanceData> &instances);

    void SetupPipeline(const Mesh *mesh, std::string_view vertex_shader_path,
                       std::string_view fragment_shader_filepath);

    void CreateFramebuffers();
    void DestroyFramebuffers();
    void CreateCommandBuffers();

    VkShaderModule CreateShaderModule(std::string_view filepath);
    void DestroyShaderModule(VkShaderModule module);

    void CreateUniformBuffers(size_t data_size);

    // Getters
    BufferManager *GetBufferManager() { return p_buffer_manager.get(); }
    FrameBufferSize GetFramebufferSize() const { return m_framebuffer_size; }
    VkDevice GetDevice() { return p_device->GetDevice(); }
    Buffer *GetStaticVertexBuffer() { return p_static_vertex_buffer.get(); };
    const std::vector<Buffer> &GetInstanceBuffers() { return m_instance_buffers; };

    void SetClearColour(const Colour<f32> &colour);

    void ReCreateSwapchain();

    void DeviceWait() { p_device->Wait(); }

private:
    struct RenderStatistics {
        u32 instances;
    };

private:
    VkRenderPass CreateRenderPass();
    void CreateFences();
    void CacheFrameBufferSize();
    void CreateInstanceBuffers();
    void InitializeImGUI();
    void DestroyImGUI();
    ImDrawData *RenderImGUI(const RenderStatistics &stats);

private:
    std::unique_ptr<VulkanInstance> p_instance;
    std::unique_ptr<VulkanDevice> p_device;
    std::unique_ptr<BufferManager> p_buffer_manager;
    std::unique_ptr<VulkanSwapchain> p_swapchain;
    std::unique_ptr<VulkanDepthResources> p_depth_resources;
    std::unique_ptr<CommandBufferManager> p_command_buffer_manager;
    std::unique_ptr<GraphicsPipeline> p_pipeline;
    std::unique_ptr<Buffer> p_static_vertex_buffer;

    GLFWwindow *p_window;
    VkRenderPass p_render_pass;
    VkShaderModule p_vertex_shader;
    VkShaderModule p_fragment_shader;
    VkCommandBuffer p_copy_command_buffer;
    VkDescriptorPool p_imgui_pool;

    VulkanQueue m_queue;
    std::vector<VkFence> m_frame_fences;
    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<VkCommandBuffer> m_command_buffers;

    std::vector<Buffer> m_uniform_buffers;
    std::vector<Buffer> m_instance_buffers;
    std::vector<void *> m_mapped_instance_data; // Persistent mapping for instance buffers

    // Frame-related variables
    FrameBufferSize m_framebuffer_size;
    u32 m_current_frame;

    VkClearColorValue m_clear_colour;
    size_t m_max_instances;
    VSyncMode m_vsync_mode;
    u32 m_vertex_count;         // Track vertex count for drawing
    u32 m_index_count;          // Track index count for indexed drawing
    bool m_use_indexed_drawing; // Flag to switch between vkCmdDraw and vkCmdDrawIndexed
    bool m_is_initialized;
};

} // namespace gouda::vk
