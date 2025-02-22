#include "gouda_vk_wrapper.hpp"

#include "gouda_assert.hpp"

#include "gouda_vk_utils.hpp"

namespace GoudaVK {

void BeginCommandBuffer(VkCommandBuffer command_buffer_ptr, VkCommandBufferUsageFlags usage_flags)
{
    VkCommandBufferBeginInfo begin_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                           .pNext = nullptr,
                                           .flags = usage_flags,
                                           .pInheritanceInfo = nullptr};

    VkResult result{vkBeginCommandBuffer(command_buffer_ptr, &begin_info)};
    CHECK_VK_RESULT(result, "vkBeginCommandBuffer\n");

    ASSERT(result == VK_SUCCESS, "Failed to begin command buffer");
}

void EndCommandBuffer(VkCommandBuffer command_buffer_ptr)
{
    VkResult result{vkEndCommandBuffer(command_buffer_ptr)};
    CHECK_VK_RESULT(result, "vkEndCommandBuffer\n");

    ASSERT(result == VK_SUCCESS, "Failed to end command buffer");
}

void CreateSemaphore(VkDevice device, VkSemaphore &semaphore)
{
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult result{vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphore)};
    CHECK_VK_RESULT(result, "vkCreateSemaphore");

    ASSERT(result == VK_SUCCESS, "Failed to create semaphore");
}

void CreateFence(VkDevice device, const VkFenceCreateInfo *create_info_ptr, const VkAllocationCallbacks *allocator_ptr,
                 VkFence *fence_ptr)
{

    VkResult result = vkCreateFence(device, create_info_ptr, nullptr, fence_ptr);
    CHECK_VK_RESULT(result, "vkCreateFence\n");

    ASSERT(result == VK_SUCCESS, "Failed to create fence");
}

} // end namespace
