#include "gouda_vk_queue.hpp"

#include <vulkan/vulkan.h>

#include "gouda_assert.hpp"
#include "gouda_vk_utils.hpp"
#include "gouda_vk_wrapper.hpp"

namespace GoudaVK {

VulkanQueue::VulkanQueue() : p_device{VK_NULL_HANDLE}, p_swap_chain{VK_NULL_HANDLE}, p_queue{VK_NULL_HANDLE}
{
    p_present_complete_semaphores.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
    p_render_complete_semaphores.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
}

VulkanQueue::~VulkanQueue() {}

void VulkanQueue::Init(VkDevice device_ptr, VkSwapchainKHR swap_chain_ptr, u32 queue_family, u32 queue_index)
{
    ASSERT(device_ptr != nullptr, "Pointer 'device_ptr' should not be null!");
    ASSERT(swap_chain_ptr != nullptr, "Pointer 'swap_chain_ptr' should not be null!");

    p_device = device_ptr;
    p_swap_chain = swap_chain_ptr;

    vkGetDeviceQueue(device_ptr, queue_family, queue_index, &p_queue);

    std::cout << "Queue acquired and initialized\n";
    CreateSemaphores();
}

void VulkanQueue::Destroy()
{
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (p_present_complete_semaphores[i]) {
            vkDestroySemaphore(p_device, p_present_complete_semaphores[i], nullptr);
            p_present_complete_semaphores[i] = VK_NULL_HANDLE;
        }
        if (p_render_complete_semaphores[i]) {
            vkDestroySemaphore(p_device, p_render_complete_semaphores[i], nullptr);
            p_render_complete_semaphores[i] = VK_NULL_HANDLE;
        }
    }
}

u32 VulkanQueue::AcquireNextImage(u32 frame_index)
{
    ASSERT(frame_index < MAX_FRAMES_IN_FLIGHT, "Frame index out of bounds!");

    u32 image_index{0};
    VkResult result{vkAcquireNextImageKHR(p_device, p_swap_chain, std::numeric_limits<u64>::max(),
                                          p_present_complete_semaphores[frame_index], VK_NULL_HANDLE, &image_index)};

    CHECK_VK_RESULT(result, "vkAcquireNextImageKHR\n");

    return image_index;
}

void VulkanQueue::Submit(VkCommandBuffer command_buffer, u32 frame_index, VkFence fence)
{
    ASSERT(frame_index < MAX_FRAMES_IN_FLIGHT, "Frame index out of bounds!");

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                             .pNext = nullptr,
                             .waitSemaphoreCount = 1,
                             .pWaitSemaphores = &p_present_complete_semaphores[frame_index],
                             .pWaitDstStageMask = &wait_stage,
                             .commandBufferCount = 1,
                             .pCommandBuffers = &command_buffer,
                             .signalSemaphoreCount = 1,
                             .pSignalSemaphores = &p_render_complete_semaphores[frame_index]};

    VkResult result = vkQueueSubmit(p_queue, 1, &submit_info, fence);
    CHECK_VK_RESULT(result, "vkQueueSubmit\n");
}

void VulkanQueue::Submit(VkCommandBuffer command_buffer, VkFence fence)
{
    VkSubmitInfo submit_info{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                             .pNext = nullptr,
                             .waitSemaphoreCount = 0,
                             .pWaitSemaphores = nullptr,
                             .pWaitDstStageMask = nullptr,
                             .commandBufferCount = 1,
                             .pCommandBuffers = &command_buffer,
                             .signalSemaphoreCount = 0,
                             .pSignalSemaphores = nullptr};

    VkResult result = vkQueueSubmit(p_queue, 1, &submit_info, fence);
    CHECK_VK_RESULT(result, "vkQueueSubmit\n");
}

void VulkanQueue::Present(u32 image_index, u32 frame_index)
{
    ASSERT(frame_index < MAX_FRAMES_IN_FLIGHT, "Frame index out of bounds!");

    VkPresentInfoKHR present_info{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                  .pNext = nullptr,
                                  .waitSemaphoreCount = 1,
                                  .pWaitSemaphores = &p_render_complete_semaphores[frame_index],
                                  .swapchainCount = 1,
                                  .pSwapchains = &p_swap_chain,
                                  .pImageIndices = &image_index,
                                  .pResults = nullptr};

    VkResult result = vkQueuePresentKHR(p_queue, &present_info);
    CHECK_VK_RESULT(result, "vkQueuePresentKHR\n");

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Handle in Application
    }
}

void VulkanQueue::WaitIdle() { vkQueueWaitIdle(p_queue); }

void VulkanQueue::CreateSemaphores()
{
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        CreateSemaphore(p_device, p_present_complete_semaphores[i]);
        CreateSemaphore(p_device, p_render_complete_semaphores[i]);
    }
}

} // end namespace