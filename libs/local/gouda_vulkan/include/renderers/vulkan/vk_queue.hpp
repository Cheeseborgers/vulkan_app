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

    void Initialize(VkDevice device, VkSwapchainKHR *swapchain, u32 queue_family, u32 queue_index);
    void Destroy();

    u32 AcquireNextImage(u32 frame_index);

    void Submit(VkCommandBuffer command_buffer, u32 frame_index, VkFence fence = VK_NULL_HANDLE); // For render loop
    void Submit(VkCommandBuffer command_buffer, VkFence fence = VK_NULL_HANDLE);                  // For standalone ops
    void SetSwapchain(VkSwapchainKHR *swapchain) { p_swapchain = swapchain; }
    void Present(u32 image_index, u32 frame_index);

    void WaitIdle();

    u32 GetMaxFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; }

private:
    void CreateSemaphores();

private:
    VkDevice p_device;
    VkSwapchainKHR *p_swapchain;
    VkQueue p_queue;

    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> p_present_complete_semaphores; // One per frame
    std::vector<VkSemaphore> p_render_complete_semaphores;  // One per fram
};

} // namesapce vk
} // namespace gouda