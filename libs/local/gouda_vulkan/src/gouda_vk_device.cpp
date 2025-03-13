#include "gouda_vk_device.hpp"

#include <print> // TODO: remove when logging fully implemented
#include <ranges>

#include "debug/assert.hpp"
#include "debug/throw.hpp"

#include "logger.hpp"

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

namespace gouda {
namespace vk {

static VkFormat FindDepthFormat(VkPhysicalDevice device_ptr)
{
    std::vector<VkFormat> candidates{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkFormat depth_format{FindSupportedFormat(device_ptr, candidates, VK_IMAGE_TILING_OPTIMAL,
                                              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)};

    return depth_format;
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

static void LogImageUsageFlags(const VkImageUsageFlags &flags)
{
    if (flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        ENGINE_LOG_DEBUG("Image usage transfer src is supported");
    }

    if (flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        ENGINE_LOG_DEBUG("Image usage transfer dest is supported");
    }

    if (flags & VK_IMAGE_USAGE_SAMPLED_BIT) {
        ENGINE_LOG_DEBUG("Image usage sampled is supported");
    }

    if (flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        ENGINE_LOG_DEBUG("Image usage color attachment is supported");
    }

    if (flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        ENGINE_LOG_DEBUG("Image usage depth stencil attachment is supported");
    }

    if (flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
        ENGINE_LOG_DEBUG("Image usage transient attachment is supported");
    }

    if (flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
        ENGINE_LOG_DEBUG("Image usage input attachment is supported");
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

// TODO: Remove these print statments and figure some other way
void VulkanPhysicalDevices::Init(const VulkanInstance &instance, const VkSurfaceKHR &surface)
{
    u32 number_of_devices{0};
    VkResult result{vkEnumeratePhysicalDevices(instance.GetInstance(), &number_of_devices, nullptr)};
    CHECK_VK_RESULT(result, "vkEnumeratePhysicalDevices error (1)\n");

    if (number_of_devices == 0) {
        ENGINE_THROW("No Vulkan physical devices found");
    }

    m_devices.resize(number_of_devices);
    std::vector<VkPhysicalDevice> devices(number_of_devices);
    result = vkEnumeratePhysicalDevices(instance.GetInstance(), &number_of_devices, devices.data());
    CHECK_VK_RESULT(result, "vkEnumeratePhysicalDevices error (2)\n");

    for (auto &&[i, physical_device] : std::views::enumerate(devices)) {
        auto &device_info = m_devices[i];
        device_info.m_physical_device = physical_device;

        vkGetPhysicalDeviceProperties(physical_device, &device_info.m_device_properties);
        ENGINE_LOG_DEBUG("Device/gpu name: {}", device_info.m_device_properties.deviceName);

        u32 api_version{device_info.m_device_properties.apiVersion};
        std::print("    API version: {}.{}.{}.{}\n", VK_API_VERSION_VARIANT(api_version),
                   VK_API_VERSION_MAJOR(api_version), VK_API_VERSION_MINOR(api_version),
                   VK_API_VERSION_PATCH(api_version));

        // Queue Families --------------------------------------------------------------------------------------
        u32 queue_family_count{0};
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        std::print("    Num of family queues: {}\n", queue_family_count);

        device_info.m_queue_family_properties.resize(queue_family_count);
        device_info.m_queue_supports_present.resize(queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                                 device_info.m_queue_family_properties.data());

        for (auto &&[q, queue_props] : std::views::enumerate(device_info.m_queue_family_properties)) {
            // VkQueueFlags flags{queue_props.queueFlags};
            //  std::print("    Family {} Num queues: {} | GFX: {}, Compute: {}, Transfer: {}, Sparse: {}\n", q,
            //             queue_props.queueCount, (flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No",
            //             (flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No", (flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" :
            //             "No", (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No");

            result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, q, surface,
                                                          &device_info.m_queue_supports_present[q]);
            CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfaceSupportKHR error\n");
        }

        // Surface Formats --------------------------------------------------------------------------------------
        u32 format_count{0};
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfaceFormatsKHR (1)\n");
        ASSERT(format_count > 0, "Number of device surface formats cannot be zero");

        device_info.m_surface_formats.resize(format_count);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count,
                                                      device_info.m_surface_formats.data());
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfaceFormatsKHR (2)\n");

        for (const auto &surface_format : device_info.m_surface_formats) {
            std::print("    Format: {}, Color Space: {}\n", VkFormatToString(surface_format.format),
                       VkColorSpaceToString(surface_format.colorSpace));
        }

        // Surface Capabilities -----------------------------------------------------------------------------------
        result =
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &device_info.m_surface_capabilities);
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR\n");
        LogImageUsageFlags(device_info.m_surface_capabilities.supportedUsageFlags);

        // Present Modes -------------------------------------------------------------------------------------------
        u32 present_mode_count{0};
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfacePresentModesKHR (1) error\n");
        ASSERT(present_mode_count != 0, "Number of device present modes cannot be zero");

        device_info.m_present_modes.resize(present_mode_count);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count,
                                                           device_info.m_present_modes.data());
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfacePresentModesKHR (2) error\n");

        std::print("Number of presentation modes: {}\n", present_mode_count);

        // Memory Properties --------------------------------------------------------------------------------------
        vkGetPhysicalDeviceMemoryProperties(physical_device, &device_info.m_memory_properties);

        /*
        std::print("Num memory types: {}\n", device_info.m_memory_properties.memoryTypeCount);

        for (auto &&[j, mem_type] : std::views::enumerate(std::span{device_info.m_memory_properties.memoryTypes,
                                                                    device_info.m_memory_properties.memoryTypeCount})) {
            std::print("{}: flags {:x}, heap {}\n", j, mem_type.propertyFlags, mem_type.heapIndex);
            PrintMemoryProperty(mem_type.propertyFlags);
        }

        std::print("Number of heap types: {}\n\n", device_info.m_memory_properties.memoryHeapCount);

        */

        // Device Features ---------------------------------------------------------------------------------------
        vkGetPhysicalDeviceFeatures(device_info.m_physical_device, &device_info.m_features);

        // Depth format ------------------------------------------------------------------------------------------
        device_info.m_depth_format = FindDepthFormat(physical_device);
    }
}

u32 VulkanPhysicalDevices::SelectDevice(VkQueueFlags required_queue_type, bool supports_present)
{
    for (u32 i = 0; i < m_devices.size(); i++) {

        for (u32 j = 0; j < m_devices[i].m_queue_family_properties.size(); j++) {
            const VkQueueFamilyProperties &queue_family_properties = m_devices[i].m_queue_family_properties[j];

            if ((queue_family_properties.queueFlags & required_queue_type) &&
                (static_cast<bool>(m_devices[i].m_queue_supports_present[j]) == supports_present)) {

                m_dev_index = static_cast<int>(i);
                u32 queue_family{j};

                ENGINE_LOG_INFO("Using GFX device: {} and queue family: {}", m_dev_index, queue_family);

                return queue_family;
            }
        }
    }

    ENGINE_LOG_ERROR("Required queue type: {} and support presents: {} not found", required_queue_type,
                     supports_present);

    return 0;
}

const PhysicalDevice &VulkanPhysicalDevices::Selected() const
{
    if (m_dev_index < 0) {
        ENGINE_LOG_ERROR("No physical device selected");
        ENGINE_THROW("No physical device selected");
    }

    return m_devices[static_cast<size_t>(m_dev_index)];
}

// VulkanDevice
// ----------------------------------------------------------------------------------------------------------
VulkanDevice::VulkanDevice(const VulkanInstance &instance, VkQueueFlags requiredQueueFlags)
    : p_device{VK_NULL_HANDLE}, m_physical_devices{}, m_queue_family{0}
{
    m_physical_devices.Init(instance, instance.GetSurface());
    m_queue_family = m_physical_devices.SelectDevice(requiredQueueFlags, true);
    CreateDevice(requiredQueueFlags);
}

VulkanDevice::~VulkanDevice()
{
    if (p_device != VK_NULL_HANDLE) {
        vkDestroyDevice(p_device, nullptr);
        ENGINE_LOG_INFO("Device destroyed");
    }
}

void VulkanDevice::CreateDevice(VkQueueFlags requiredQueueFlags)
{
    float queue_priorities[]{1.0f};
    VkDeviceQueueCreateInfo device_queue_create_info{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                     .queueFamilyIndex = m_queue_family,
                                                     .queueCount = 1,
                                                     .pQueuePriorities = queue_priorities};

    std::vector<const char *> device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME};

    VkPhysicalDeviceFeatures physical_device_features{};
    physical_device_features.geometryShader = VK_TRUE;
    physical_device_features.tessellationShader = VK_TRUE;

    VkDeviceCreateInfo device_create_info{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                          .queueCreateInfoCount = 1,
                                          .pQueueCreateInfos = &device_queue_create_info,
                                          .enabledExtensionCount = static_cast<u32>(device_extensions.size()),
                                          .ppEnabledExtensionNames = device_extensions.data(),
                                          .pEnabledFeatures = &physical_device_features};

    VkResult result{
        vkCreateDevice(m_physical_devices.Selected().m_physical_device, &device_create_info, nullptr, &p_device)};
    CHECK_VK_RESULT(result, "vkCreateDevice\n");

    // Log feature support for debugging
    if (m_physical_devices.Selected().m_features.geometryShader == VK_FALSE) {
        ENGINE_LOG_ERROR("The Geometry Shader is not supported!");
    }
    if (m_physical_devices.Selected().m_features.tessellationShader == VK_FALSE) {
        ENGINE_LOG_ERROR("The Tessellation Shader is not supported!");
    }

    ENGINE_LOG_INFO("Device created with queue family index: {}", m_queue_family);
}

} // namespace vk
} // namespace gouda