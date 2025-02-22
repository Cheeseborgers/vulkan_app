#pragma once

#include <cstdint>

#include <vulkan/vulkan.h>

namespace GoudaVK {

class AllocatedBuffer {
public:
    AllocatedBuffer();

    VkBuffer p_buffer;
    VkDeviceMemory p_memory;
    VkDeviceSize m_allocation_size;

    void Update(VkDevice device_ptr, const void *data_ptr, std::size_t size);

    void Destroy(VkDevice device_ptr);
};

} // end namespace