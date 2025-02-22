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

    u32 AcquireNextImage();

    void SubmitSync(VkCommandBuffer command_buffer_ptr);
    void SubmitAsync(VkCommandBuffer command_buffer_ptr);

    void FramePresent(u32 image_index);

    void WaitIdle();

private:
    void CreateSemaphores();

private:
    VkDevice p_device;
    VkSwapchainKHR p_swap_chain;
    VkQueue p_queue;
    VkSemaphore p_render_complete_semaphore;
    VkSemaphore p_present_complete_semaphore;
};

} // end namespace