#pragma once
/**
 * @file vk_texture.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-24
 * @brief Engine Vulkan texture module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <vulkan/vulkan.h>

#include "core/types.hpp"

namespace gouda::vk {

class Device;


struct Texture {
    Texture();

    void Destroy(Device *device);

    VkImage p_image;
    VkDeviceMemory p_memory;
    VkImageView p_view;
    VkSampler p_sampler;
};

[[nodiscard]] VkSampler create_texture_sampler(Device *device, VkFilter min_filter, VkFilter max_filter,
                                               VkSamplerAddressMode address_mode);

[[nodiscard]] constexpr int get_bytes_per_texture_format(VkFormat Format) noexcept;

[[nodiscard]] constexpr bool has_stencil_component(const VkFormat Format) noexcept
{
    return ((Format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (Format == VK_FORMAT_D24_UNORM_S8_UINT));
}

[[nodiscard]] constexpr VkFormat image_channels_to_vk_format(const int channels)
{
    switch (channels) {
        case 1:
            return VK_FORMAT_R8_UNORM;
        case 3:
            return VK_FORMAT_R8G8B8_UNORM;
        case 4:
            return VK_FORMAT_R8G8B8A8_UNORM;
        default:
            return VK_FORMAT_UNDEFINED;
    }
}

[[nodiscard]] constexpr u32 vk_format_to_channel_count(const VkFormat format)
{
    switch (format) {
        case VK_FORMAT_R8_UNORM:
            return 2;
        case VK_FORMAT_R8G8B8_UNORM:
            return 3;
        case VK_FORMAT_R8G8B8A8_UNORM:
            return 4;
        default:
            return 0;
    }
}

} // namespace gouda::vk