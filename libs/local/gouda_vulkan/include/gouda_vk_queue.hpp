#pragma once

#include <vulkan/vulkan.h>

#include "gouda_types.hpp"

namespace GoudaVK {

class VulkanQueue {

public:
    VulkanQueue();
    ~VulkanQueue();

    void Init(VkDevice device_ptr, VkSwapchainKHR swap_chain_ptr, u32 queue_family, u32 queue_index);
    void Destroy();

    u32 AcquireNextImage(u32 frame_index);

    void Submit(VkCommandBuffer command_buffer, u32 frame_index, VkFence fence = VK_NULL_HANDLE); // For render loop
    void Submit(VkCommandBuffer command_buffer, VkFence fence = VK_NULL_HANDLE);                  // For standalone ops

    void Present(u32 image_index, u32 frame_index);

    void WaitIdle();

    u32 GetMaxFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; } // Expose to Application

private:
    void CreateSemaphores();

private:
    VkDevice p_device;
    VkSwapchainKHR p_swap_chain;
    VkQueue p_queue;

    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> p_present_complete_semaphores; // One per frame
    std::vector<VkSemaphore> p_render_complete_semaphores;  // One per frame
};

} // end namespace