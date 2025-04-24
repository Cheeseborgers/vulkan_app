#pragma once
/**
 * @file vk_swapchain.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-19
 * @brief Engine module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <atomic>

#include <vulkan/vulkan.h>

#include "core/types.hpp"

struct GLFWwindow;

namespace gouda::vk {

class Device;
class Instance;
class BufferManager;

enum class VSyncMode : u8 {
    Disabled, // Uncapped FPS
    Enabled,  // Standard V-Sync (reduces tearing, may cause input lag)
    Mailbox   // Adaptive V-Sync (triple-buffering, minimizes lag & tearing)
};

class Swapchain {
public:
    Swapchain(GLFWwindow *window, Device *device, Instance *instance, BufferManager *buffer_manager,
              VSyncMode vsync_mode);
    ~Swapchain();

    void CreateSwapchain();
    void CreateSwapchainImageViews();

    void DestroySwapchain();
    void DestroySwapchainImageViews();

    void Recreate();

    [[nodiscard]] VkSwapchainKHR *Get() { return &p_swap_chain; }
    [[nodiscard]] const std::vector<VkImage> &GetImages() const { return m_images; }
    [[nodiscard]] size_t GetImageCount() const { return m_images.size(); }
    [[nodiscard]] const std::vector<VkImageView> &GetImageViews() const { return m_image_views; }
    [[nodiscard]] VkExtent2D GetExtent() { return m_extent; }
    [[nodiscard]] FrameBufferSize GetFramebufferSize() { return FrameBufferSize(m_extent.width, m_extent.height); }
    [[nodiscard]] VkSurfaceFormatKHR GetSurfaceFormat() { return m_surface_format; }

    void SetInvalid() noexcept { m_is_valid.store(false, std::memory_order_release); }
    void SetValid() noexcept { m_is_valid.store(true, std::memory_order_release); }
    bool IsValid() const noexcept { return m_is_valid.load(std::memory_order_acquire); }

private:
    void UpdateExtent();

private:
    VkSwapchainKHR p_swap_chain;
    VkSurfaceFormatKHR m_surface_format;
    VSyncMode m_vsync_mode;
    u32 m_queue_family;

    GLFWwindow *p_window;

    Device *p_device;
    Instance *p_instance;
    BufferManager *p_buffer_manager;

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;

    VkExtent2D m_extent;

    std::atomic<bool> m_is_valid;
};

} // namespace gouda::vk