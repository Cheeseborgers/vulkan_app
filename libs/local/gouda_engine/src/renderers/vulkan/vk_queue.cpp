#include "renderers/vulkan/vk_queue.hpp"

#include <vulkan/vulkan.h>

#include "debug/assert.hpp"
#include "debug/logger.hpp"
#include "renderers/vulkan/gouda_vk_wrapper.hpp"
#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_fence.hpp"
#include "renderers/vulkan/vk_swapchain.hpp"
#include "renderers/vulkan/vk_utils.hpp"
#include "renderers/vulkan/vk_semaphore.hpp"

namespace gouda::vk {

Queue::Queue() : p_device{nullptr}, p_swapchain{nullptr}, p_queue{VK_NULL_HANDLE}
{
}

Queue::~Queue() { Destroy(); }

void Queue::Initialize(Device *device, Swapchain *swapchain, const u32 queue_family, const u32 queue_index)
{
    p_device = device;
    p_swapchain = swapchain;

    vkGetDeviceQueue(device->GetDevice(), queue_family, queue_index, &p_queue);

    p_present_complete_semaphores.resize(p_swapchain->GetImageCount());
    p_render_complete_semaphores.resize(p_swapchain->GetImageCount());
    CreateSemaphores();
}

void Queue::Destroy()
{
    if (p_device) {
        for (auto &semaphore : p_present_complete_semaphores) {
            vkDestroySemaphore(p_device->GetDevice(), semaphore, nullptr);
            semaphore = VK_NULL_HANDLE;
        }

        ENGINE_LOG_DEBUG("Present complete semaphores destroyed");

        for (auto &semaphore : p_render_complete_semaphores) {
            vkDestroySemaphore(p_device->GetDevice(), semaphore, nullptr);
            semaphore = VK_NULL_HANDLE;
        }

        ENGINE_LOG_DEBUG("Render complete semaphores destroyed");
    }

    ENGINE_LOG_DEBUG("Queue destroyed.");
}

u32 Queue::AcquireNextImage(const u32 frame_index)
{
    ASSERT(frame_index < MAX_FRAMES_IN_FLIGHT, "Frame index out of bounds!");

    u32 image_index{0};
    const VkResult result{vkAcquireNextImageKHR(p_device->GetDevice(), *p_swapchain->Get(), constants::u64_max,
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
        return constants::u32_max; // Special value indicating swapchain recreation is needed
    }

    if (result == VK_ERROR_DEVICE_LOST) {
        ENGINE_LOG_ERROR("vkAcquireNextImageKHR returned VK_ERROR_DEVICE_LOST. Device lost!");
        // HandleDeviceLost(); // Handle device loss (usually requires application restart)
        return constants::u32_max;
    }

    if (result == VK_ERROR_SURFACE_LOST_KHR) {
        ENGINE_LOG_ERROR("vkAcquireNextImageKHR returned VK_ERROR_SURFACE_LOST_KHR. Surface lost!");
        // HandleSurfaceLost();
        return constants::u32_max;
    }

    // Catch all other unknown errors
    ENGINE_LOG_ERROR("vkAcquireNextImageKHR failed with error: {}", vk_result_to_string(result));
    return constants::u32_max; // Return invalid index to prevent further errors
}

void Queue::Submit(const VkCommandBuffer command_buffer, const u32 frame_index, const Fence *fence)
{
    ASSERT(frame_index < MAX_FRAMES_IN_FLIGHT, "Frame index out of bounds!");

    constexpr VkPipelineStageFlags wait_stage{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &p_present_complete_semaphores[frame_index];
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &p_render_complete_semaphores[frame_index];

    if (const VkResult result{vkQueueSubmit(p_queue, 1, &submit_info, fence->Get())}; result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkQueueSubmit");
    }
}

void Queue::Submit(const VkCommandBuffer command_buffer, const Fence *fence) const
{
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    if (const VkResult result{vkQueueSubmit(p_queue, 1, &submit_info, fence->Get())}; result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkQueueSubmit");
    }
}

void Queue::Present(const u32 image_index, const u32 frame_index)
{
    ASSERT(frame_index < MAX_FRAMES_IN_FLIGHT, "Frame index out of bounds!");

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &p_render_complete_semaphores[frame_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = p_swapchain->Get();
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;

    if (const VkResult result{vkQueuePresentKHR(p_queue, &present_info)};
        result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        ENGINE_LOG_ERROR("Swapchain outdated after presentation, recreating...");
    }
    else {
        CHECK_VK_RESULT(result, "vkQueuePresentKHR");
    }
}

void Queue::WaitIdle() const { vkQueueWaitIdle(p_queue); }

void Queue::CreateSemaphores()
{
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        CreateSemaphore(p_device->GetDevice(), p_present_complete_semaphores[i]);
        CreateSemaphore(p_device->GetDevice(), p_render_complete_semaphores[i]);
    }
}

} // namespace gouda::vk