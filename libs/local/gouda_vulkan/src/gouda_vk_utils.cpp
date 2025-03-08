#include <cstdio>
#include <cstdlib>
#include <format>
#include <vector>

#include <vulkan/vulkan.h>

#include "gouda_types.hpp"
#include "gouda_vk_utils.hpp"

void PrintVKExtensions()
{
    u32 extension_count{0};
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    std::cout << "Available vulkan extensions:\n";

    for (const auto &extension : extensions) {
        std::cout << '\t' << extension.extensionName << '\n';
    }
}

const char *GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT Severity)
{
    switch (Severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return "Verbose";

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            return "Info";

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            return "Warning";

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            return "Error";

        default:
            // Log error to console for now
            std::fprintf(stderr, "Invalid severity code - %d\n", static_cast<int>(Severity));
            exit(1); // Exit program or handle error appropriately
    }

    return "NO SUCH SEVERITY!";
}

const char *GetDebugType(VkDebugUtilsMessageTypeFlagsEXT Type)
{
    switch (Type) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            return "General";

        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            return "Validation";

        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            return "Performance";

        case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
            return "Device address binding";

        default:
            // Log error to console for now
            std::fprintf(stderr, "Invalid type code - %d\n", static_cast<int>(Type));
            exit(1);
    }

    return "NO SUCH TYPE!";
}

uint32_t GetBytesPerTexFormat(VkFormat Format)
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

VkFormat FindSupportedFormat(VkPhysicalDevice Device, const std::vector<VkFormat> &Candidates, VkImageTiling Tiling,
                             VkFormatFeatureFlags Features)
{
    for (int i = 0; i < Candidates.size(); i++) {
        VkFormat Format = Candidates[i];
        VkFormatProperties Props;
        vkGetPhysicalDeviceFormatProperties(Device, Format, &Props);

        if ((Tiling == VK_IMAGE_TILING_LINEAR) && (Props.linearTilingFeatures & Features) == Features) {
            return Format;
        }
        else if (Tiling == VK_IMAGE_TILING_OPTIMAL && (Props.optimalTilingFeatures & Features) == Features) {
            return Format;
        }
    }

    printf("Failed to find supported format!\n");
    exit(1);
}
