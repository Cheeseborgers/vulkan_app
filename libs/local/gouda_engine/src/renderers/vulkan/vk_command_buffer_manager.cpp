/**
 * @file vk_command_buffer_manager.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-19
 * @brief Engine Vulkan command buffer manager implementation
 */
#include "renderers/vulkan/vk_command_buffer_manager.hpp"

#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_queue.hpp"

#include "debug/logger.hpp"
#include "renderers/vulkan/vk_utils.hpp"

namespace gouda::vk {

CommandBufferManager::CommandBufferManager(Device *device, Queue *queue, u32 queue_family)
    : p_device{device}, p_queue{queue}, m_queue_family{queue_family}, p_pool{VK_NULL_HANDLE}
{
    CreatePool();
}

CommandBufferManager::~CommandBufferManager()
{
    if (p_pool != VK_NULL_HANDLE && p_device != nullptr) {
        vkDestroyCommandPool(p_device->GetDevice(), p_pool, nullptr);
        ENGINE_LOG_DEBUG("Command pool destroyed.");
    }
}

void CommandBufferManager::CreatePool()
{
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.pNext = nullptr;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = m_queue_family;

    if (const VkResult result{vkCreateCommandPool(p_device->GetDevice(), &pool_info, nullptr, &p_pool)};
        result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateCommandPool");
    }

    ENGINE_LOG_DEBUG("Command buffer pool created.");
}

void CommandBufferManager::AllocateBuffers(u32 count, VkCommandBuffer *buffers) const
{
    const VkCommandBufferAllocateInfo alloc_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                           .pNext = nullptr,
                                           .commandPool = p_pool,
                                           .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                           .commandBufferCount = count};

    if (const VkResult result{vkAllocateCommandBuffers(p_device->GetDevice(), &alloc_info, buffers)}; result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkAllocateCommandBuffers");
    }

    ENGINE_LOG_DEBUG("Command buffer(s) created: {}.", count);
}

void CommandBufferManager::FreeBuffers(u32 count, const VkCommandBuffer *buffers) const
{
    if (p_device != nullptr && p_queue != nullptr) {
        p_queue->WaitIdle();
        vkFreeCommandBuffers(p_device->GetDevice(), p_pool, count, buffers);
        ENGINE_LOG_DEBUG("Command buffer(s) freed: {}.", count);
    }
}

} // namespace gouda::vk
