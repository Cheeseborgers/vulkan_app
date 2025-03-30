#pragma once

#include <memory>

#include <vulkan/vulkan.h>

#include "core/types.hpp"
#include "renderers/common.hpp"
#include "renderers/vulkan/vk_buffer.hpp"
#include "renderers/vulkan/vk_texture.hpp"

namespace gouda::vk {

struct Mesh {
    Mesh();
    void Destroy(VkDevice device_ptr);

    Buffer m_vertex_buffer;
    size_t m_vertex_buffer_size;
    PrimitiveTopology m_topology;
    std::unique_ptr<VulkanTexture> p_texture;
};

} // namespace gouda::vk