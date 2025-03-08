#include "gouda_vk_device.hpp"

#include <format>
#include <print>
#include <ranges>
#include <span>
#include <unordered_map>
#include <vector>

#include "gouda_assert.hpp"

#include "gouda_vk_utils.hpp"

template <>
struct std::formatter<VkFormat> : std::formatter<std::string> {
    auto format(VkFormat format, std::format_context &ctx) const
    {
        return std::format_to(ctx.out(), "{:#x}", static_cast<u32>(format));
    }
};

template <>
struct std::formatter<VkColorSpaceKHR> : std::formatter<std::string> {
    auto format(VkColorSpaceKHR colorspace, std::format_context &ctx) const
    {
        return std::format_to(ctx.out(), "{:#x}", static_cast<u32>(colorspace));
    }
};

namespace GoudaVK {

static VkFormat FindDepthFormat(VkPhysicalDevice Device)
{
    std::vector<VkFormat> Candidates = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                                        VK_FORMAT_D24_UNORM_S8_UINT};

    VkFormat DepthFormat = FindSupportedFormat(Device, Candidates, VK_IMAGE_TILING_OPTIMAL,
                                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    return DepthFormat;
}

// Vulkan Format to String Map
std::string VkFormatToString(VkFormat format)
{
    static const std::unordered_map<VkFormat, std::string> format_map = {
        {VK_FORMAT_UNDEFINED, "VK_FORMAT_UNDEFINED"},
        {VK_FORMAT_R8G8B8A8_UNORM, "VK_FORMAT_R8G8B8A8_UNORM"},
        {VK_FORMAT_B8G8R8A8_UNORM, "VK_FORMAT_B8G8R8A8_UNORM"},
        {VK_FORMAT_R8G8B8A8_SRGB, "VK_FORMAT_R8G8B8A8_SRGB"},
        {VK_FORMAT_B8G8R8A8_SRGB, "VK_FORMAT_B8G8R8A8_SRGB"},
        {VK_FORMAT_D32_SFLOAT, "VK_FORMAT_D32_SFLOAT"},
        {VK_FORMAT_D24_UNORM_S8_UINT, "VK_FORMAT_D24_UNORM_S8_UINT"},
        {VK_FORMAT_R16G16B16A16_SFLOAT, "VK_FORMAT_R16G16B16A16_SFLOAT"},
        {VK_FORMAT_R32G32B32A32_SFLOAT, "VK_FORMAT_R32G32B32A32_SFLOAT"},
        {VK_FORMAT_R5G6B5_UNORM_PACK16, "VK_FORMAT_R5G6B5_UNORM_PACK16"},
        // Add more as needed
    };

    if (auto it = format_map.find(format); it != format_map.end()) {
        return it->second;
    }
    return "Unknown VkFormat (" + std::to_string(static_cast<u32>(format)) + ")";
}

// Vulkan Color Space to String Map
std::string VkColorSpaceToString(VkColorSpaceKHR color_space)
{
    static const std::unordered_map<VkColorSpaceKHR, std::string> color_space_map = {
        {VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR"},
        {VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT, "VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT"},
        {VK_COLOR_SPACE_HDR10_ST2084_EXT, "VK_COLOR_SPACE_HDR10_ST2084_EXT"},
        {VK_COLOR_SPACE_DCI_P3_LINEAR_EXT, "VK_COLOR_SPACE_DCI_P3_LINEAR_EXT"},
        {VK_COLOR_SPACE_BT709_LINEAR_EXT, "VK_COLOR_SPACE_BT709_LINEAR_EXT"},
        {VK_COLOR_SPACE_BT709_NONLINEAR_EXT, "VK_COLOR_SPACE_BT709_NONLINEAR_EXT"},
        {VK_COLOR_SPACE_HDR10_HLG_EXT, "VK_COLOR_SPACE_HDR10_HLG_EXT"},
        {VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT, "VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT"},
        {VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT, "VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT"},
        // Add more as needed
    };

    if (auto it = color_space_map.find(color_space); it != color_space_map.end()) {
        return it->second;
    }
    return "Unknown VkColorSpace (" + std::to_string(static_cast<u32>(color_space)) + ")";
}

static void PrintImageUsageFlags(const VkImageUsageFlags &flags)
{
    if (flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        std::cout << "Image usage transfer src is supported\n";
    }

    if (flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        std::cout << "Image usage transfer dest is supported\n";
    }

    if (flags & VK_IMAGE_USAGE_SAMPLED_BIT) {
        std::cout << "Image usage sampled is supported\n";
    }

    if (flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        std::cout << "Image usage color attachment is supported\n";
    }

    if (flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        std::cout << "Image usage depth stencil attachment is supported\n";
    }

    if (flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
        std::cout << "Image usage transient attachment is supported\n";
    }

    if (flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
        std::cout << "Image usage input attachment is supported\n";
    }
}

static void PrintMemoryProperty(VkMemoryPropertyFlags PropertyFlags)
{
    if (PropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        std::cout << "DEVICE LOCAL ";
    }

    if (PropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        std::cout << "HOST VISIBLE ";
    }

    if (PropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        std::cout << "HOST COHERENT ";
    }

    if (PropertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
        std::cout << "HOST CACHED ";
    }

    if (PropertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
        std::cout << "LAZILY ALLOCATED ";
    }

    if (PropertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT) {
        std::cout << "PROTECTED ";
    }

    std::cout << std::endl; // Ensures the output ends with a newline
}

// TODO: REMove these print statments and figure some other way
void VulkanPhysicalDevices::Init(const VkInstance &instance, const VkSurfaceKHR &surface)
{
    u32 number_of_devices{0};

    VkResult result = vkEnumeratePhysicalDevices(instance, &number_of_devices, nullptr);
    CHECK_VK_RESULT(result, "vkEnumeratePhysicalDevices error (1)\n");

    std::print("Num physical devices: {}\n\n", number_of_devices);
    m_devices.resize(number_of_devices);

    std::vector<VkPhysicalDevice> devices(number_of_devices);
    result = vkEnumeratePhysicalDevices(instance, &number_of_devices, devices.data());
    CHECK_VK_RESULT(result, "vkEnumeratePhysicalDevices error (2)\n");

    for (auto &&[i, physical_device] : std::views::enumerate(devices)) {
        auto &device_info = m_devices[i];
        device_info.m_phys_device = physical_device;

        vkGetPhysicalDeviceProperties(physical_device, &device_info.m_dev_props);
        std::print("Device name: {}\n", device_info.m_dev_props.deviceName);

        u32 api_version = device_info.m_dev_props.apiVersion;
        std::print("    API version: {}.{}.{}.{}\n", VK_API_VERSION_VARIANT(api_version),
                   VK_API_VERSION_MAJOR(api_version), VK_API_VERSION_MINOR(api_version),
                   VK_API_VERSION_PATCH(api_version));

        // Queue Families
        u32 queue_family_count{0};
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        std::print("    Num of family queues: {}\n", queue_family_count);

        device_info.m_queue_family_props.resize(queue_family_count);
        device_info.m_queue_supports_present.resize(queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                                 device_info.m_queue_family_props.data());

        for (auto &&[q, queue_props] : std::views::enumerate(device_info.m_queue_family_props)) {
            VkQueueFlags flags = queue_props.queueFlags;
            std::print("    Family {} Num queues: {} | GFX: {}, Compute: {}, Transfer: {}, Sparse: {}\n", q,
                       queue_props.queueCount, (flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No",
                       (flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No", (flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No",
                       (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No");

            result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, q, surface,
                                                          &device_info.m_queue_supports_present[q]);
            CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfaceSupportKHR error\n");
        }

        // Surface Formats
        u32 num_formats{0};
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, nullptr);
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfaceFormatsKHR (1)\n");
        ASSERT(num_formats > 0, "Number of device surface formats cannot be zero");

        device_info.m_surface_formats.resize(num_formats);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats,
                                                      device_info.m_surface_formats.data());
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfaceFormatsKHR (2)\n");

        for (const auto &surface_format : device_info.m_surface_formats) {
            std::print("    Format: {}, Color Space: {}\n", VkFormatToString(surface_format.format),
                       VkColorSpaceToString(surface_format.colorSpace));
        }

        // Surface Capabilities
        result =
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &device_info.m_surface_capabilties);
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR\n");
        PrintImageUsageFlags(device_info.m_surface_capabilties.supportedUsageFlags);

        // Present Modes
        u32 num_present_modes{0};
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, nullptr);
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfacePresentModesKHR (1) error\n");
        ASSERT(num_present_modes != 0, "Number of device present modes cannot be zero");

        device_info.m_present_modes.resize(num_present_modes);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes,
                                                           device_info.m_present_modes.data());
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfacePresentModesKHR (2) error\n");

        std::print("Number of presentation modes: {}\n", num_present_modes);

        // Memory Properties
        vkGetPhysicalDeviceMemoryProperties(physical_device, &device_info.m_memory_props);
        std::print("Num memory types: {}\n", device_info.m_memory_props.memoryTypeCount);

        for (auto &&[j, mem_type] : std::views::enumerate(
                 std::span{device_info.m_memory_props.memoryTypes, device_info.m_memory_props.memoryTypeCount})) {
            std::print("{}: flags {:x}, heap {}\n", j, mem_type.propertyFlags, mem_type.heapIndex);
            PrintMemoryProperty(mem_type.propertyFlags);
        }

        std::print("Num heap types: {}\n\n", device_info.m_memory_props.memoryHeapCount);

        // Device Features
        vkGetPhysicalDeviceFeatures(device_info.m_phys_device, &device_info.m_features);
    }
}

u32 VulkanPhysicalDevices::SelectDevice(VkQueueFlags required_queue_type, bool supports_present)
{
    for (u32 i = 0; i < m_devices.size(); i++) {

        for (u32 j = 0; j < m_devices[i].m_queue_family_props.size(); j++) {
            const VkQueueFamilyProperties &queue_family_properties = m_devices[i].m_queue_family_props[j];

            if ((queue_family_properties.queueFlags & required_queue_type) &&
                (static_cast<bool>(m_devices[i].m_queue_supports_present[j]) == supports_present)) {

                m_dev_index = static_cast<int>(i);
                u32 queue_family{j};

                // TODO: Add to logger when created
                std::cout << "Using GFX device " << m_dev_index << " and queue family " << queue_family << '\n';

                return queue_family;
            }
        }
    }

    // TODO: Add to logger when created
    std::cout << "Required queue type  " << required_queue_type << " and supports present " << supports_present
              << ' not found\n';

    return 0;
}

const PhysicalDevice &VulkanPhysicalDevices::Selected() const
{
    if (m_dev_index < 0) {
        // TODO: Add to logger when created
        std::cout << "A physical device has not been selected\n";
    }

    return m_devices[static_cast<size_t>(m_dev_index)];
}

} // end namespace