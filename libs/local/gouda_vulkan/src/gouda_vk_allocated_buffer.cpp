#include "gouda_vk_allocated_buffer.hpp"

#include <cstring> // For memcpy

#include "gouda_vk_utils.hpp"

namespace GoudaVK {

AllocatedBuffer::AllocatedBuffer() : p_buffer{nullptr}, p_memory{nullptr}, m_allocation_size{0} {}

void AllocatedBuffer::Update(VkDevice device_ptr, const void *data_ptr, std::size_t size)
{
    void *memory_ptr{nullptr};
    VkResult result = vkMapMemory(device_ptr, p_memory, 0, size, 0, &memory_ptr);
    CHECK_VK_RESULT(result, "vkMapMemory");
    memcpy(memory_ptr, data_ptr, size);
    vkUnmapMemory(device_ptr, p_memory);
}

void AllocatedBuffer::Destroy(VkDevice device_ptr)
{
    if (p_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_ptr, p_buffer, nullptr);
        p_buffer = VK_NULL_HANDLE;
    }

    if (p_memory != VK_NULL_HANDLE) {
        vkFreeMemory(device_ptr, p_memory, nullptr);
        p_memory = VK_NULL_HANDLE;
    }
}
}