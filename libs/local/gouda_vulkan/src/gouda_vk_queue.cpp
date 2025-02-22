#include "gouda_vk_queue.hpp"

#include <vulkan/vulkan.h>

#include "gouda_assert.hpp"
#include "gouda_vk_utils.hpp"
#include "gouda_vk_wrapper.hpp"

namespace GoudaVK {

VulkanQueue::VulkanQueue()
    : p_device{VK_NULL_HANDLE},
      p_swap_chain{VK_NULL_HANDLE},
      p_queue{VK_NULL_HANDLE},
      p_render_complete_semaphore{VK_NULL_HANDLE},
      p_present_complete_semaphore{VK_NULL_HANDLE}
{
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
    if (p_present_complete_semaphore) {
        vkDestroySemaphore(p_device, p_present_complete_semaphore, nullptr);
        p_present_complete_semaphore = VK_NULL_HANDLE; // Prevent dangling handle
    }

    if (p_render_complete_semaphore) {
        vkDestroySemaphore(p_device, p_render_complete_semaphore, nullptr);
        p_render_complete_semaphore = VK_NULL_HANDLE;
    }
}

u32 VulkanQueue::AcquireNextImage()
{
    u32 image_index{0};
    VkResult result{vkAcquireNextImageKHR(p_device, p_swap_chain, std::numeric_limits<u64>::max(),
                                          p_present_complete_semaphore, nullptr, &image_index)};

    CHECK_VK_RESULT(result, "vkAcquireNextImageKHR\n");

    return image_index;
}

void VulkanQueue::SubmitSync(VkCommandBuffer command_buffer)
{
    VkSubmitInfo submit_info{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                             .pNext = nullptr,
                             .waitSemaphoreCount = 0,
                             .pWaitSemaphores = VK_NULL_HANDLE,
                             .pWaitDstStageMask = VK_NULL_HANDLE,
                             .commandBufferCount = 1,
                             .pCommandBuffers = &command_buffer,
                             .signalSemaphoreCount = 0,
                             .pSignalSemaphores = VK_NULL_HANDLE};

    VkResult results{vkQueueSubmit(p_queue, 1, &submit_info, nullptr)};
    CHECK_VK_RESULT(results, "vkQueueSubmit\n");
}

void VulkanQueue::SubmitAsync(VkCommandBuffer command_buffer)
{
    VkPipelineStageFlags wait_flags{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                             .pNext = nullptr,
                             .waitSemaphoreCount = 1,
                             .pWaitSemaphores = &p_present_complete_semaphore,
                             .pWaitDstStageMask = &wait_flags,
                             .commandBufferCount = 1,
                             .pCommandBuffers = &command_buffer,
                             .signalSemaphoreCount = 1,
                             .pSignalSemaphores = &p_render_complete_semaphore};

    VkResult result{vkQueueSubmit(p_queue, 1, &submit_info, nullptr)};
    CHECK_VK_RESULT(result, "vkQueueSubmit\n");
}

// TODO: Rename to FramePresent
void VulkanQueue::FramePresent(u32 image_index)
{
    VkPresentInfoKHR present_info{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                  .pNext = nullptr,
                                  .waitSemaphoreCount = 1,
                                  .pWaitSemaphores = &p_render_complete_semaphore,
                                  .swapchainCount = 1,
                                  .pSwapchains = &p_swap_chain,
                                  .pImageIndices = &image_index,
                                  .pResults = nullptr};

    VkResult result{vkQueuePresentKHR(p_queue, &present_info)};
    CHECK_VK_RESULT(result, "vkQueuePresentKHR\n");

    // FIXME: What are we going to do here?
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // g_SwapChainRebuild = true;
    }

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return;
    }

    if (result != VK_SUBOPTIMAL_KHR) {
        CHECK_VK_RESULT(result, "");
    }

    WaitIdle(); // TODO: looks like a workaround but we're getting error messages without it
}

void VulkanQueue::WaitIdle() { vkQueueWaitIdle(p_queue); }

void VulkanQueue::CreateSemaphores()
{
    CreateSemaphore(p_device, p_present_complete_semaphore);
    CreateSemaphore(p_device, p_render_complete_semaphore);
}

} // end namespace