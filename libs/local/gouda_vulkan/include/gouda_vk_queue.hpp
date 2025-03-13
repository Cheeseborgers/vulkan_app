#pragma once

#include <atomic>

#include <vulkan/vulkan.h>

#include "core/types.hpp"

namespace gouda {
namespace vk {

class VulkanQueue {

public:
    VulkanQueue();
    ~VulkanQueue();

    void Init(VkDevice device_ptr, VkSwapchainKHR *swap_chain_ptr, u32 queue_family, u32 queue_index);
    void Destroy();

    u32 AcquireNextImage(u32 frame_index);

    void Submit(VkCommandBuffer command_buffer, u32 frame_index, VkFence fence = VK_NULL_HANDLE); // For render loop
    void Submit(VkCommandBuffer command_buffer, VkFence fence = VK_NULL_HANDLE);                  // For standalone ops

    void Present(u32 image_index, u32 frame_index);

    void WaitIdle();

    u32 GetMaxFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; }

    void SetSwapchainInvalid() { m_swapchain_valid.store(false, std::memory_order_release); }
    void SetSwapchainValid() { m_swapchain_valid.store(true, std::memory_order_release); }
    bool IsSwapchainValid() const { return m_swapchain_valid.load(std::memory_order_acquire); }

private:
    void CreateSemaphores();

private:
    VkDevice p_device;
    VkSwapchainKHR *p_swap_chain;
    VkQueue p_queue;

    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> p_present_complete_semaphores; // One per frame
    std::vector<VkSemaphore> p_render_complete_semaphores;  // One per frame

    std::atomic<bool> m_swapchain_valid;
};

} // namesapce vk
} // namespace gouda