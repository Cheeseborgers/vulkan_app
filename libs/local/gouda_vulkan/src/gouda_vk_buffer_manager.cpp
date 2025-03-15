/**
 * @file gouda_vk_buffer_manager.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-15
 * @brief Engine module implementation
 */
#include "gouda_vk_buffer_manager.hpp"
#include "debug/throw.hpp"
#include "gouda_vk_core.hpp" // For VulkanDevice

#include "logger.hpp"

namespace gouda {
namespace vk {

BufferManager::BufferManager(VulkanDevice *device) : p_device(device), p_copy_command_buffer{VK_NULL_HANDLE}
{
    VkCommandPoolCreateInfo pool_info{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                      .queueFamilyIndex = p_device->GetQueueFamily()};
    VkResult result = vkCreateCommandPool(p_device->GetDevice(), &pool_info, nullptr, &p_command_pool);
    CHECK_VK_RESULT(result, "vkCreateCommandPool in BufferManager");

    VkCommandBufferAllocateInfo alloc_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                           .commandPool = p_command_pool,
                                           .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                           .commandBufferCount = 1};
    result = vkAllocateCommandBuffers(p_device->GetDevice(), &alloc_info, &p_copy_command_buffer);
    CHECK_VK_RESULT(result, "vkAllocateCommandBuffers in BufferManager");
}

BufferManager::~BufferManager()
{
    if (p_copy_command_buffer != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(p_device->GetDevice(), p_command_pool, 1, &p_copy_command_buffer);
        ENGINE_LOG_INFO("BufferManager command buffer freed.");
    }
    if (p_command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(p_device->GetDevice(), p_command_pool, nullptr);
        ENGINE_LOG_INFO("BufferManager command pool destroyed.");
    }
}

Buffer BufferManager::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo buffer_info{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                   .size = size,
                                   .usage = usage,
                                   .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

    Buffer buffer{};
    VkResult result = vkCreateBuffer(p_device->GetDevice(), &buffer_info, nullptr, &buffer.p_buffer);
    CHECK_VK_RESULT(result, "vkCreateBuffer");

    VkMemoryRequirements mem_requirements{};
    vkGetBufferMemoryRequirements(p_device->GetDevice(), buffer.p_buffer, &mem_requirements);
    buffer.m_allocation_size = mem_requirements.size;

    auto memory_type_index = GetMemoryTypeIndex(mem_requirements.memoryTypeBits, properties);
    if (!memory_type_index) {
        ENGINE_THROW("Memory type selection failed: {}", memory_type_index.error());
    }

    VkMemoryAllocateInfo alloc_info{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                    .allocationSize = mem_requirements.size,
                                    .memoryTypeIndex = *memory_type_index};
    result = vkAllocateMemory(p_device->GetDevice(), &alloc_info, nullptr, &buffer.p_memory);
    CHECK_VK_RESULT(result, "vkAllocateMemory");

    vkBindBufferMemory(p_device->GetDevice(), buffer.p_buffer, buffer.p_memory, 0);
    return buffer;
}
Buffer BufferManager::CreateVertexBuffer(const void *vertices_ptr, size_t size)
{
    Buffer staging_buffer = CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    staging_buffer.Update(p_device->GetDevice(), vertices_ptr, size);

    Buffer vertex_buffer = CreateBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CopyBufferToBuffer(vertex_buffer.p_buffer, staging_buffer.p_buffer, size, p_copy_command_buffer);

    VkQueue queue;
    vkGetDeviceQueue(p_device->GetDevice(), p_device->GetQueueFamily(), 0, &queue);
    vkQueueWaitIdle(queue);

    staging_buffer.Destroy(p_device->GetDevice());
    return vertex_buffer;
}
Buffer BufferManager::CreateUniformBuffer(size_t size)
{
    return CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void BufferManager::CreateImage(VulkanTexture &texture, VkImageCreateInfo &image_info,
                                VkMemoryPropertyFlags memory_properties)
{
    VkResult result = vkCreateImage(p_device->GetDevice(), &image_info, nullptr, &texture.p_image);
    CHECK_VK_RESULT(result, "vkCreateImage");

    VkMemoryRequirements memory_requirements{};
    vkGetImageMemoryRequirements(p_device->GetDevice(), texture.p_image, &memory_requirements);

    auto memory_type_index = GetMemoryTypeIndex(memory_requirements.memoryTypeBits, memory_properties);
    if (!memory_type_index) {
        ENGINE_THROW("Memory type selection failed: {}", memory_type_index.error());
    }

    VkMemoryAllocateInfo memory_allocate_info{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                              .allocationSize = memory_requirements.size,
                                              .memoryTypeIndex = *memory_type_index};

    result = vkAllocateMemory(p_device->GetDevice(), &memory_allocate_info, nullptr, &texture.p_memory);
    CHECK_VK_RESULT(result, "vkAllocateMemory");

    vkBindImageMemory(p_device->GetDevice(), texture.p_image, texture.p_memory, 0);
}

void BufferManager::CopyBufferToBuffer(VkBuffer destination, VkBuffer source, VkDeviceSize size,
                                       VkCommandBuffer command_buffer)
{
    BeginCommandBuffer(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferCopy copy_region{.srcOffset = 0, .dstOffset = 0, .size = size};
    vkCmdCopyBuffer(command_buffer, source, destination, 1, &copy_region);

    SubmitCopyCommand(command_buffer);
}

Expect<u32, std::string> BufferManager::GetMemoryTypeIndex(u32 memory_type_bits,
                                                           VkMemoryPropertyFlags required_properties)
{
    const VkPhysicalDeviceMemoryProperties &mem_properties = p_device->GetSelectedPhysicalDevice().m_memory_properties;
    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((memory_type_bits & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & required_properties) == required_properties) {
            return i;
        }
    }
    return std::unexpected("Cannot find memory type for type: " + std::to_string(memory_type_bits));
}

void BufferManager::BeginCommandBuffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage)
{
    VkCommandBufferBeginInfo begin_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = usage};
    vkBeginCommandBuffer(command_buffer, &begin_info);
}

void BufferManager::SubmitCopyCommand(VkCommandBuffer command_buffer)
{
    vkEndCommandBuffer(command_buffer);

    VkQueue queue;
    vkGetDeviceQueue(p_device->GetDevice(), p_device->GetQueueFamily(), 0, &queue);
    VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &command_buffer};
    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
}

} // namespace vk
} // namespace gouda