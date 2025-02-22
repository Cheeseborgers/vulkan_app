#include "gouda_vk_wrapper.hpp"
#include "gouda_vk_utils.hpp"

namespace GoudaVK {

void BeginCommandBuffer(VkCommandBuffer CommandBuffer, VkCommandBufferUsageFlags UsageFlags)
{
    VkCommandBufferBeginInfo begin_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                           .pNext = nullptr,
                                           .flags = UsageFlags,
                                           .pInheritanceInfo = nullptr};

    VkResult result{vkBeginCommandBuffer(CommandBuffer, &begin_info)};
    CHECK_VK_RESULT(result, "vkBeginCommandBuffer\n");
}

void CreateSemaphore(VkDevice device, VkSemaphore &semaphore)
{
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult result{vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphore)};
    CHECK_VK_RESULT(result, "vkCreateSemaphore");
}

void CreateFence(VkDevice device, VkFence &fence)
{
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled to allow first frame submission

    VkResult result{vkCreateFence(device, &fence_create_info, nullptr, &fence)};
    CHECK_VK_RESULT(result, "Failed to create Vulkan fence.");
}

} // end namespace
