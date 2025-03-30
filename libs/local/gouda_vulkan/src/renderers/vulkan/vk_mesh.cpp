#include "renderers/vulkan/vk_mesh.hpp"

#include "debug/logger.hpp"

namespace gouda::vk {

Mesh::Mesh()
    : m_vertex_buffer{}, m_vertex_buffer_size{0}, m_topology{PrimitiveTopology::TriangleList}, p_texture{nullptr}
{
    ENGINE_LOG_DEBUG("Mesh created");
}

void Mesh::Destroy(VkDevice device_ptr)
{
    if (p_texture) {
        p_texture->Destroy(device_ptr);
        p_texture.reset();
        ENGINE_LOG_DEBUG("Mesh texture destroyed");
    }
    ENGINE_LOG_DEBUG("Mesh destroyed");
}

} // namespace gouda::vk