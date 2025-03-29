#include "renderers/vulkan/vk_queue.hpp"

#include <vulkan/vulkan.h>

#include "debug/assert.hpp"
#include "debug/logger.hpp"
#include "renderers/vulkan/gouda_vk_wrapper.hpp"
#include "renderers/vulkan/vk_utils.hpp"

namespace gouda::vk {

VulkanQueue::VulkanQueue() : p_device{VK_NULL_HANDLE}, p_swapchain{nullptr}, p_queue{VK_NULL_HANDLE}
{
    p_present_complete_semaphores.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
    p_render_complete_semaphores.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
}

VulkanQueue::~VulkanQueue() { Destroy(); }

void VulkanQueue::Initialize(VkDevice device, VkSwapchainKHR *swapchain, u32 queue_family, u32 queue_index)
{
    p_device = device;
    p_swapchain = swapchain;

    vkGetDeviceQueue(device, queue_family, queue_index, &p_queue);

    ENGINE_LOG_DEBUG("Queue initialized");

    CreateSemaphores();
}

void VulkanQueue::Destroy()
{
    if (p_device) {
        for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (p_present_complete_semaphores[i]) {
                vkDestroySemaphore(p_device, p_present_complete_semaphores[i], nullptr);
                p_present_complete_semaphores[i] = VK_NULL_HANDLE;
                ENGINE_LOG_DEBUG("Present complete semaphore {} destroyed", i);
            }
            if (p_render_complete_semaphores[i]) {
                vkDestroySemaphore(p_device, p_render_complete_semaphores[i], nullptr);
                p_render_complete_semaphores[i] = VK_NULL_HANDLE;
                ENGINE_LOG_DEBUG("Render complete semaphore {} destroyed", i);
            }
        }
    }

    ENGINE_LOG_DEBUG("Queue destroyed.");
}

u32 VulkanQueue::AcquireNextImage(u32 frame_index)
{
    ASSERT(frame_index < MAX_FRAMES_IN_FLIGHT, "Frame index out of bounds!");

    u32 image_index{0};
    VkResult result{vkAcquireNextImageKHR(p_device, *p_swapchain, Constants::u64_max,
                                          p_present_complete_semaphores[frame_index], VK_NULL_HANDLE, &image_index)};

    if (result == VK_SUCCESS) {
        return image_index;
    }

    if (result == VK_SUBOPTIMAL_KHR) {
        // Swapchain is still usable, but should be recreated soon
        ENGINE_LOG_WARNING("vkAcquireNextImageKHR returned VK_SUBOPTIMAL_KHR. Consider recreating swapchain.");
        return image_index;
    }

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Swapchain is no longer valid and must be recreated
        ENGINE_LOG_WARNING("vkAcquireNextImageKHR returned VK_ERROR_OUT_OF_DATE_KHR. Swapchain recreation needed.");
        // HandleSwapchainRecreation();
        return Constants::u32_max; // Special value indicating swapchain recreation is needed
    }

    if (result == VK_ERROR_DEVICE_LOST) {
        ENGINE_LOG_ERROR("vkAcquireNextImageKHR returned VK_ERROR_DEVICE_LOST. Device lost!");
        // HandleDeviceLost(); // Handle device loss (usually requires application restart)
        return Constants::u32_max;
    }

    if (result == VK_ERROR_SURFACE_LOST_KHR) {
        ENGINE_LOG_ERROR("vkAcquireNextImageKHR returned VK_ERROR_SURFACE_LOST_KHR. Surface lost!");
        // HandleSurfaceLost();
        return Constants::u32_max;
    }

    // Catch all other unknown errors
    ENGINE_LOG_ERROR("vkAcquireNextImageKHR failed with error: {}", VKResultToString(result));
    return Constants::u32_max; // Return invalid index to prevent further errors
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

    VkResult result{vkQueueSubmit(p_queue, 1, &submit_info, fence)};
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

    VkResult result{vkQueueSubmit(p_queue, 1, &submit_info, fence)};
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
                                  .pSwapchains = p_swapchain,
                                  .pImageIndices = &image_index,
                                  .pResults = nullptr};

    VkResult result{vkQueuePresentKHR(p_queue, &present_info)};
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        ENGINE_LOG_ERROR("Swapchain outdated after presentation, recreating...");
    }
    else {
        CHECK_VK_RESULT(result, "vkQueuePresentKHR");
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

} // namespace gouda::vk