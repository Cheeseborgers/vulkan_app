#include "renderers/vulkan/vk_mesh.hpp"

namespace gouda::vk {

Mesh::Mesh() : p_texture{nullptr} {}

void Mesh::Destroy(VkDevice device_ptr)
{
    if (p_texture) {
        p_texture->Destroy(device_ptr);
        p_texture.reset();
    }
}

} // namespace gouda::vk