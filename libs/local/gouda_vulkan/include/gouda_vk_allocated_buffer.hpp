#pragma once

#include <cstdint>

#include <vulkan/vulkan.h>

namespace gouda {
namespace vk {

class AllocatedBuffer {
public:
    AllocatedBuffer();

    VkBuffer p_buffer;
    VkDeviceMemory p_memory;
    VkDeviceSize m_allocation_size;

    void Update(VkDevice device_ptr, const void *data_ptr, std::size_t size);

    void Destroy(VkDevice device_ptr);
};

} // namespace vk
} // namespace gouda