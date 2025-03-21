// Copyright (c) 2025 GoodeCheeseburgers <https://github.com/Cheeseborgers>
//
// This file is part of gouda_vulkan.
//
// gouda_vulkan is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
#pragma once

#include <expected>
#include <string_view>

// #include "backends/imgui_impl_glfw.h"
// #include "backends/imgui_impl_vulkan.h"
// #include "imgui.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/types.hpp"

#include "gouda_vk_buffer_manager.hpp"
#include "gouda_vk_device.hpp"
#include "gouda_vk_instance.hpp"
#include "gouda_vk_queue.hpp"
#include "gouda_vk_texture.hpp"

// TODO: Think about how we are going to handle application exiting on errors
// TODO: Implement image loading with 'gouda_image'

namespace gouda {
namespace vk {

enum class VSyncMode : u8 {
    DISABLED, // Uncapped FPS
    ENABLED,  // Standard V-Sync (reduces tearing, may cause input lag)
    MAILBOX   // Adaptive V-Sync (triple-buffering, minimizes lag & tearing)
};

class VulkanCore {

public:
    VulkanCore();
    ~VulkanCore();

    void Init(GLFWwindow *window_ptr, std::string_view app_name, SemVer vulkan_api_version = {1, 3, 0, 0},
              VSyncMode v_sync = gouda::vk::VSyncMode::ENABLED);

    VkRenderPass CreateSimpleRenderPass();

    std::vector<VkFramebuffer> CreateFramebuffers(VkRenderPass render_pass_ptr) const;
    void DestroyFramebuffers(std::vector<VkFramebuffer> &frame_buffers);

    void CreateCommandBuffers(u32 count, VkCommandBuffer *command_buffers_ptr);
    void FreeCommandBuffers(u32 count, const VkCommandBuffer *command_buffers_ptr);

    Buffer CreateVertexBuffer(const void *vertices_ptr, size_t data_size);

    // TODO: Remove one of these functions
    void CreateUniformBuffers(std::vector<Buffer> &uniform_buffers, size_t data_size);
    std::vector<Buffer> CreateUniformBuffers(size_t data_size);

    void CreateTexture(std::string_view file_name, VulkanTexture &texture);

    // Getters
    VulkanQueue *GetQueue() { return &m_queue; }
    u32 GetQueueFamily() const { return p_device->GetQueueFamily(); }
    int GetNumberOfImages() const { return static_cast<int>(m_images.size()); }
    const VkImage &GetImage(int index);
    VkDevice GetDevice() { return p_device->GetDevice(); }
    void GetFramebufferSize(FrameBufferSize &size) const;

    void DestroySwapchain();
    void ReCreateSwapchain();

    void DeviceWait() { p_device->Wait(); }

private:
    void CacheFrameBufferSize();
    void CreateSwapchain();
    void CreateSwapchainImageViews();
    void DestroySwapchainImageViews();
    void CreateDepthResources();
    void DestroyDepthResources();
    void CreateCommandBufferPool();
    void CreateTextureImageFromData(VulkanTexture &texture, const void *pixels_ptr, ImageSize image_size,
                                    VkFormat texture_format, u32 layer_count, VkImageCreateFlags create_flags);
    void CreateImage(VulkanTexture &texture, ImageSize image_size, VkFormat texture_format, VkImageTiling image_tiling,
                     VkImageUsageFlags usage_flags, VkMemoryPropertyFlagBits memory_property_flags,
                     VkImageCreateFlags create_flags, u32 mip_levels);
    void CreateTextureImage(VulkanTexture &texture, ImageSize size, VkFormat format, u32 mip_levels, u32 layer_count,
                            VkImageCreateFlags create_flags, VkImageUsageFlags additional_usage_flags = 0);
    void CreateDepthImage(VulkanTexture &texture, ImageSize size, VkFormat format);
    void UpdateTextureImage(VulkanTexture &texture, ImageSize image_size, VkFormat texture_format, u32 layer_count,
                            const void *pixels_ptr, VkImageLayout source_image_layout);
    void CopyBufferToImage(VkImage destination, VkBuffer source, ImageSize image_size, u32 layer_count);
    void TransitionImageLayout(VkImage &image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout,
                               u32 layer_count, u32 mip_levels);
    void TransitionImageLayoutCmd(VkCommandBuffer command_buffer_ptr, VkImage image, VkFormat format,
                                  VkImageLayout old_layout, VkImageLayout new_layout, u32 layer_count, u32 mip_levels);

    void SubmitCopyCommand();

private:
    bool m_is_initialized;
    VSyncMode m_vsync_mode;

    std::unique_ptr<VulkanInstance> p_instance;
    std::unique_ptr<VulkanDevice> p_device;
    std::unique_ptr<BufferManager> p_buffer_manager;

    GLFWwindow *p_window;

    FrameBufferSize m_frame_buffer_size;

    VkSurfaceFormatKHR m_swap_chain_surface_format;
    VkSwapchainKHR p_swap_chain;

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;
    std::vector<VulkanTexture> m_depth_images;

    VkCommandPool p_command_buffer_pool;
    VulkanQueue m_queue;

    VkCommandBuffer p_copy_command_buffer;
};

} // namespace vk
} // namespace gouda