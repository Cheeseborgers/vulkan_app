#include <cstdio>
#include <cstdlib>
#include <format>
#include <vector>

#include <vulkan/vulkan.h>

#include "logger.hpp"

#include "core/types.hpp"
#include "debug/throw.hpp"
#include "gouda_vk_utils.hpp"

namespace gouda {
namespace vk {

void PrintVKExtensions()
{
    u32 extension_count{0};
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    // TODO: Figure how we are going to log this better
    std::cout << "Available vulkan extensions:\n";

    for (const auto &extension : extensions) {
        std::cout << '\t' << extension.extensionName << '\n';
    }
}

// TODO: Look into returning expected?
const char *GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return "Verbose";

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            return "Info";

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            return "Warning";

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            return "Error";

        default:
            ENGINE_THROW("Invalid debug message severity code: {}.", static_cast<int>(severity));
    }

    return "No such severity!";
}

// TODO: Look into returning expected?
const char *GetDebugType(VkDebugUtilsMessageTypeFlagsEXT type)
{
    switch (type) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            return "General";

        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            return "Validation";

        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            return "Performance";

        case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
            return "Device address binding";

        default:
            ENGINE_THROW("Invalid debug type code: {}.", static_cast<int>(type));
    }

    return "No such type!";
}

u32 GetBytesPerTexFormat(VkFormat Format)
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

VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features)
{
    for (const auto &format : candidates) {
        VkFormatProperties format_properties{};
        vkGetPhysicalDeviceFormatProperties(device, format, &format_properties);

        if ((tiling == VK_IMAGE_TILING_LINEAR) && (format_properties.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                 (format_properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    ENGINE_THROW("Failed to find supported format!");
}

} // namespace vk
} // namespace gouda
