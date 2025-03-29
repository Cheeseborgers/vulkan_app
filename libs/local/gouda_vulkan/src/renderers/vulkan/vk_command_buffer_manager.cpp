/**
 * @file vk_command_buffer_manager.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-19
 * @brief Engine module implementation
 */

#include "renderers/vulkan/vk_command_buffer_manager.hpp"

#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_queue.hpp"

#include "debug/logger.hpp"

namespace gouda::vk {

CommandBufferManager::CommandBufferManager(VulkanDevice *device, VulkanQueue *queue, u32 queue_family)
    : p_device{device}, p_queue{queue}, m_queue_family{queue_family}
{
    CreatePool();
}

CommandBufferManager::~CommandBufferManager()
{
    if (p_pool != VK_NULL_HANDLE && p_device != VK_NULL_HANDLE) {
        vkDestroyCommandPool(p_device->GetDevice(), p_pool, nullptr);
        ENGINE_LOG_DEBUG("Command pool destroyed.");
    }
}

void CommandBufferManager::CreatePool()
{
    VkCommandPoolCreateInfo pool_info{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                      .pNext = nullptr,
                                      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                      .queueFamilyIndex = m_queue_family};

    VkResult result{vkCreateCommandPool(p_device->GetDevice(), &pool_info, nullptr, &p_pool)};
    CHECK_VK_RESULT(result, "vkCreateCommandPool\n");
    ENGINE_LOG_DEBUG("Command buffer pool created.");
}

void CommandBufferManager::AllocateBuffers(u32 count, VkCommandBuffer *buffers)
{
    VkCommandBufferAllocateInfo alloc_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                           .pNext = nullptr,
                                           .commandPool = p_pool,
                                           .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                           .commandBufferCount = count};

    VkResult result{vkAllocateCommandBuffers(p_device->GetDevice(), &alloc_info, buffers)};
    CHECK_VK_RESULT(result, "vkAllocateCommandBuffers\n");
    ENGINE_LOG_DEBUG("Command buffer(s) created: {}.", count);
}

void CommandBufferManager::FreeBuffers(u32 count, const VkCommandBuffer *buffers)
{
    if (p_device != nullptr && p_queue != nullptr) {
        p_queue->WaitIdle();
        vkFreeCommandBuffers(p_device->GetDevice(), p_pool, count, buffers);
        ENGINE_LOG_DEBUG("Command buffer(s) freed: {}.", count);
    }
}

} // namespace gouda::vk
