#pragma once

#include <vulkan/vulkan.h>

#include "core/types.hpp"
#include "gouda_vk_core.hpp"

namespace gouda {
namespace vk {

struct SimpleMesh {
    AllocatedBuffer m_vertex_buffer;
    std::size_t m_vertex_buffer_size;
    VulkanTexture *p_texture;

    SimpleMesh();

    void Destroy(VkDevice device_ptr);
};

} // namesapce vk
} // namespace gouda