#include "renderers/vulkan/vk_texture.hpp"

#include <vulkan/vulkan.h>

#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_utils.hpp"

namespace gouda::vk {

Sprite::Sprite() : looping{true} {}

Texture::Texture()
    : p_image{VK_NULL_HANDLE}, p_memory{VK_NULL_HANDLE}, p_view{VK_NULL_HANDLE}, p_sampler{VK_NULL_HANDLE}
{
}

void Texture::Destroy(const Device *device)
{
    if (device) {
        if (p_sampler) {
            vkDestroySampler(device->GetDevice(), p_sampler, nullptr);
            p_sampler = VK_NULL_HANDLE;
        }

        if (p_view) {
            vkDestroyImageView(device->GetDevice(), p_view, nullptr);
            p_view = VK_NULL_HANDLE;
        }

        if (p_image) {
            vkDestroyImage(device->GetDevice(), p_image, nullptr);
            p_image = VK_NULL_HANDLE;
        }

        if (p_memory) {
            vkFreeMemory(device->GetDevice(), p_memory, nullptr);
            p_memory = VK_NULL_HANDLE;
        }
    }
}

TextureMetadata::TextureMetadata() : is_atlas{false}, texture{nullptr} {}
TextureMetadata::~TextureMetadata() { sprites.clear(); }

// Function declarations --------------------------------------------------------------
[[nodiscard]] VkSampler create_texture_sampler(const Device *device, const VkFilter min_filter, const VkFilter max_filter,
                                               const VkSamplerAddressMode address_mode)
{
    const VkSamplerCreateInfo sampler_create_info{.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                                  .pNext = nullptr,
                                                  .flags = 0,
                                                  .magFilter = min_filter,
                                                  .minFilter = max_filter,
                                                  .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                                  .addressModeU = address_mode,
                                                  .addressModeV = address_mode,
                                                  .addressModeW = address_mode,

                                                  .mipLodBias = 0.0f,
                                                  .anisotropyEnable = VK_FALSE,
                                                  .maxAnisotropy = 1,
                                                  .compareEnable = VK_FALSE,
                                                  .compareOp = VK_COMPARE_OP_ALWAYS,
                                                  .minLod = 0.0f,
                                                  .maxLod = 0.0f,
                                                  .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                                                  .unnormalizedCoordinates = VK_FALSE};

    VkSampler sampler{VK_NULL_HANDLE};
    if (const VkResult result{vkCreateSampler(device->GetDevice(), &sampler_create_info, VK_NULL_HANDLE, &sampler)};
        result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateSampler");
    }

    return sampler;
}

[[nodiscard]] constexpr int get_bytes_per_texture_format(const VkFormat Format) noexcept
{
    switch (Format) {
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_UNORM:
            return 1;
        case VK_FORMAT_R16_SFLOAT:
            return 2;
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
            return 4;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 4 * sizeof(u16);
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 4 * sizeof(f32);
        default:
            return 0;
    }
}

} // namespace gouda::vk