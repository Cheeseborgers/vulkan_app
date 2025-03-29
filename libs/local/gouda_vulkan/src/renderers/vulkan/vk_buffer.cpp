#include "renderers/vulkan/vk_buffer.hpp"

#include <cstring> // For memcpy

#include "renderers/vulkan/vk_utils.hpp"

namespace gouda::vk {

Buffer::Buffer() : p_buffer{nullptr}, p_memory{nullptr}, m_allocation_size{0} {}

void *Buffer::MapPersistent(VkDevice device)
{
    void *data;
    vkMapMemory(device, p_memory, 0, m_allocation_size, 0, &data);
    return data;
}

void Buffer::Update(VkDevice device, const void *data, size_t size)
{
    void *mapped{nullptr};
    VkResult result{vkMapMemory(device, p_memory, 0, size, 0, &mapped)};
    CHECK_VK_RESULT(result, "vkMapMemory");
    memcpy(mapped, data, size);
    vkUnmapMemory(device, p_memory);
}

void Buffer::Destroy(VkDevice device)
{
    if (p_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, p_buffer, nullptr);
        p_buffer = VK_NULL_HANDLE;
    }

    if (p_memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, p_memory, nullptr);
        p_memory = VK_NULL_HANDLE;
    }
}

} // namespace gouda::vk
