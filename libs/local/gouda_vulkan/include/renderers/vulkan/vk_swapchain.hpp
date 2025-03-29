#pragma once

/**
 * @file vk_swapchain.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-19
 * @brief Engine module
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
#include <atomic>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/types.hpp"

namespace gouda {
namespace vk {

enum class VSyncMode : u8 {
    DISABLED, // Uncapped FPS
    ENABLED,  // Standard V-Sync (reduces tearing, may cause input lag)
    MAILBOX   // Adaptive V-Sync (triple-buffering, minimizes lag & tearing)
};

class VulkanDevice;
class VulkanInstance;
class BufferManager;

class VulkanSwapchain {
public:
    VulkanSwapchain(GLFWwindow *window, VulkanDevice *device, VulkanInstance *instance, BufferManager *buffer_manager,
                    VSyncMode vsync_mode);
    ~VulkanSwapchain();

    void CreateSwapchain();
    void CreateSwapchainImageViews();

    void DestroySwapchain();
    void DestroySwapchainImageViews();

    void Recreate();

    VkSwapchainKHR *GetHandle() { return &p_swap_chain; }
    const std::vector<VkImage> &GetImages() const { return m_images; }
    size_t GetImageCount() const { return m_images.size(); }
    const std::vector<VkImageView> &GetImageViews() const { return m_image_views; }
    VkExtent2D GetExtent() { return m_extent; }
    FrameBufferSize GetFramebufferSize() { return FrameBufferSize(m_extent.width, m_extent.height); }
    VkSurfaceFormatKHR GetSurfaceFormat() { return m_surface_format; }

    void SetInvalid() { m_is_valid.store(false, std::memory_order_release); } // Remove?
    void SetValid() { m_is_valid.store(true, std::memory_order_release); }    // Remove?
    bool IsValid() const { return m_is_valid.load(std::memory_order_acquire); }

private:
    void UpdateExtent();

private:
    VkSwapchainKHR p_swap_chain;
    VkSurfaceFormatKHR m_surface_format;
    VSyncMode m_vsync_mode;
    u32 m_queue_family;

    GLFWwindow *p_window;
    VulkanDevice *p_device;
    VulkanInstance *p_instance;
    BufferManager *p_buffer_manager;

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;

    VkExtent2D m_extent;

    std::atomic<bool> m_is_valid;
};

}
} // namespace gouda::vk