#include "gouda_vk_simple_mesh.hpp"

namespace gouda {
namespace vk {

SimpleMesh::SimpleMesh() : p_texture{nullptr} {}

void SimpleMesh::Destroy(VkDevice device_ptr)
{
    m_vertex_buffer.Destroy(device_ptr);

    if (p_texture) {
        p_texture->Destroy(device_ptr);
    }
}

} // namespace vk
} // namespace gouda