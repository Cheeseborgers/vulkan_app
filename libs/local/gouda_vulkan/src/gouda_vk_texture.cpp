#include "gouda_vk_texture.hpp"

#include <vulkan/vulkan.h>

#include "gouda_vk_utils.hpp"

namespace GoudaVK {

VulkanTexture::VulkanTexture()
    : p_image{VK_NULL_HANDLE}, p_memory{VK_NULL_HANDLE}, p_view{VK_NULL_HANDLE}, p_sampler{VK_NULL_HANDLE}
{
}

void VulkanTexture::Destroy(VkDevice device)
{
    if (p_sampler) {
        vkDestroySampler(device, p_sampler, nullptr);
        p_sampler = VK_NULL_HANDLE;
    }

    if (p_view) {
        vkDestroyImageView(device, p_view, nullptr);
        p_view = VK_NULL_HANDLE;
    }

    if (p_image) {
        vkDestroyImage(device, p_image, nullptr);
        p_image = VK_NULL_HANDLE;
    }

    if (p_memory) {
        vkFreeMemory(device, p_memory, nullptr);
        p_memory = VK_NULL_HANDLE;
    }
}

// Function declarations

VkSampler CreateTextureSampler(VkDevice device_ptr, VkFilter min_filter, VkFilter max_filter,
                               VkSamplerAddressMode address_mode)
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

    VkSampler sampler;
    VkResult result{vkCreateSampler(device_ptr, &sampler_create_info, VK_NULL_HANDLE, &sampler)};
    CHECK_VK_RESULT(result, "vkCreateSampler");

    return sampler;
}

int GetBytesPerTextureFormat(VkFormat Format)
{
    switch (Format) {
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_UNORM:
            return 1;
        case VK_FORMAT_R16_SFLOAT:
            return 2;
        case VK_FORMAT_R16G16_SFLOAT:
            return 4;
        case VK_FORMAT_R16G16_SNORM:
            return 4;
        case VK_FORMAT_B8G8R8A8_UNORM:
            return 4;
        case VK_FORMAT_R8G8B8A8_UNORM:
            return 4;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 4 * sizeof(u16);
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 4 * sizeof(f32);
        default:
            break;
    }

    return 0;
}

bool HasStencilComponent(VkFormat Format)
{
    return ((Format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (Format == VK_FORMAT_D24_UNORM_S8_UINT));
}

} // end namespace