/**
 * @file renderers/vulkan/gouda_vk_wrapper.cpp
 * @author GoudaCheeseburgers
 * @date 2025-04-24
 * @brief Engine Vulkan wrapper implementation
 */
#include "renderers/vulkan/gouda_vk_wrapper.hpp"

#include "renderers/vulkan/vk_utils.hpp"

namespace gouda::vk {

void BeginCommandBuffer(VkCommandBuffer command_buffer_ptr, VkCommandBufferUsageFlags usage_flags)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = usage_flags;
    begin_info.pInheritanceInfo = nullptr;

    VkResult result{vkBeginCommandBuffer(command_buffer_ptr, &begin_info)};
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkBeginCommandBuffer");
    }
}

void EndCommandBuffer(VkCommandBuffer command_buffer_ptr)
{
    VkResult result{vkEndCommandBuffer(command_buffer_ptr)};
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkEndCommandBuffer");
    }
}

void CreateSemaphore(VkDevice device, VkSemaphore &semaphore)
{
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult result{vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphore)};
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateSemaphore");
    }
}

void CreateFence(VkDevice device, const VkFenceCreateInfo *create_info_ptr, const VkAllocationCallbacks *allocator_ptr,
                 VkFence *fence_ptr)
{
    VkResult result{vkCreateFence(device, create_info_ptr, nullptr, fence_ptr)};
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateFence");
    }
}

} // namespace gouda::vk
