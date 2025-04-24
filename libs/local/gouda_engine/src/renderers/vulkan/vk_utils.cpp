#include <cstdio>
#include <cstdlib>
#include <format>
#include <vector>

#include <vulkan/vulkan.h>

#include "core/types.hpp"
#include "debug/logger.hpp"
#include "debug/throw.hpp"
#include "renderers/vulkan/vk_utils.hpp"

namespace gouda::vk {

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

std::string_view vk_result_to_string(VkResult result)
{
    switch (result) {
        case VkResult::VK_SUCCESS:
            return "VK_SUCCESS";
        case VkResult::VK_NOT_READY:
            return "VK_NOT_READY";
        case VkResult::VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VkResult::VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VkResult::VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VkResult::VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VkResult::VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VkResult::VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VkResult::VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VkResult::VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VkResult::VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VkResult::VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VkResult::VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VkResult::VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VkResult::VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VkResult::VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VkResult::VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VkResult::VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VkResult::VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VkResult::VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VkResult::VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VkResult::VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VkResult::VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VkResult::VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VkResult::VK_ERROR_NOT_PERMITTED:
            return "VK_ERROR_NOT_PERMITTED";
        case VkResult::VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VkResult::VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VkResult::VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VkResult::VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VkResult::VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VkResult::VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VkResult::VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VkResult::VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VkResult::VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VkResult::VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VkResult::VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VkResult::VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VkResult::VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VkResult::VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VkResult::VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VkResult::VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VkResult::VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VkResult::VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VkResult::VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VkResult::VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
        case VkResult::VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VkResult::VK_INCOMPATIBLE_SHADER_BINARY_EXT:
            return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
        case VkResult::VK_PIPELINE_BINARY_MISSING_KHR:
            return "VK_PIPELINE_BINARY_MISSING_KHR";
        case VkResult::VK_ERROR_NOT_ENOUGH_SPACE_KHR:
            return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
        case VkResult::VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
        default:
            return "Unknown VkResult";
    }
}

} // namespace gouda::vk
