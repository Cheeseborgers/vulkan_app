#pragma once

#include <vulkan/vulkan.h>

#include "core/types.hpp"
#include "math/vector.hpp"

namespace gouda {
namespace vk {

class VulkanTexture {
public:
    VulkanTexture();

    VkImage p_image;
    VkDeviceMemory p_memory;
    VkImageView p_view;
    VkSampler p_sampler;

    void Destroy(VkDevice device);
};

VkSampler CreateTextureSampler(VkDevice device_ptr, VkFilter min_filter, VkFilter max_filter,
                               VkSamplerAddressMode address_mode);

int GetBytesPerTextureFormat(VkFormat Format);

bool HasStencilComponent(VkFormat Format);

} // namesapce vk
} // namespace gouda