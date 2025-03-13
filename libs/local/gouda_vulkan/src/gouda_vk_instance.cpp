#include "gouda_vk_instance.hpp"

#include "debug/throw.hpp"
#include "logger.hpp"

namespace gouda {
namespace vk {

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                    VkDebugUtilsMessageTypeFlagsEXT type,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *callback_data_ptr,
                                                    [[maybe_unused]] void *user_data_ptr)
{
    std::ostringstream log;
    log << "Debug callback: " << callback_data_ptr->pMessage << '\n'
        << "  Severity: " << GetDebugSeverityStr(severity) << '\n'
        << "  Type: " << GetDebugType(type) << '\n'
        << "  Objects: ";
    for (u32 i = 0; i < callback_data_ptr->objectCount; i++) {
        log << std::hex << callback_data_ptr->pObjects[i].objectHandle << ' ';
    }
    log << "\n---------------------------------------\n";
    ENGINE_LOG_ERROR(log.str());
    return VK_FALSE;
}

VulkanInstance::VulkanInstance(std::string_view app_name, SemVer vulkan_api_version, GLFWwindow *window)
    : p_instance{VK_NULL_HANDLE}, p_debug_messenger{VK_NULL_HANDLE}, p_surface{VK_NULL_HANDLE}, p_window(window)
{
    CreateInstance(app_name, vulkan_api_version);
    CreateDebugCallback();
    CreateSurface();
}

VulkanInstance::~VulkanInstance()
{
    if (p_instance != VK_NULL_HANDLE) {
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(p_instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (vkDestroyDebugUtilsMessenger) {
            vkDestroyDebugUtilsMessenger(p_instance, p_debug_messenger, nullptr);
        }
        vkDestroySurfaceKHR(p_instance, p_surface, nullptr);
        vkDestroyInstance(p_instance, nullptr);
        ENGINE_LOG_INFO("VulkanInstance destroyed");
    }
}

void VulkanInstance::CreateInstance(std::string_view app_name, SemVer vulkan_api_version)
{
    std::vector<const char *> layers{"VK_LAYER_KHRONOS_validation"};
    std::vector<const char *> extensions{
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
        "VK_KHR_win32_surface",
#endif
#if defined(__APPLE__)
        "VK_MVK_macos_surface",
#endif
#if defined(__linux__)
        "VK_KHR_xcb_surface",
#endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };

    VkApplicationInfo application_info{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                       .pApplicationName = app_name.data(),
                                       .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
                                       .pEngineName = "Gouda Engine",
                                       .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
                                       .apiVersion =
                                           VK_MAKE_API_VERSION(vulkan_api_version.variant, vulkan_api_version.major,
                                                               vulkan_api_version.minor, vulkan_api_version.patch)};

    VkInstanceCreateInfo instance_create_info{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                              .pApplicationInfo = &application_info,
                                              .enabledLayerCount = static_cast<u32>(layers.size()),
                                              .ppEnabledLayerNames = layers.data(),
                                              .enabledExtensionCount = static_cast<u32>(extensions.size()),
                                              .ppEnabledExtensionNames = extensions.data()};

    VkResult result{vkCreateInstance(&instance_create_info, nullptr, &p_instance)};
    CHECK_VK_RESULT(result, "vkCreateInstance");
    ENGINE_LOG_INFO("Vulkan instance created");
}

void VulkanInstance::CreateDebugCallback()
{
    VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = &DebugCallback};

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(p_instance, "vkCreateDebugUtilsMessengerEXT"));
    if (!vkCreateDebugUtilsMessenger) {
        ENGINE_THROW("Cannot find address of vkCreateDebugUtilsMessengerEXT");
    }

    VkResult result{vkCreateDebugUtilsMessenger(p_instance, &messenger_create_info, nullptr, &p_debug_messenger)};
    CHECK_VK_RESULT(result, "debug utils messenger");
    ENGINE_LOG_INFO("Vulkan debug utils messenger created");
}

void VulkanInstance::CreateSurface()
{
    if (p_surface != VK_NULL_HANDLE) {
        ENGINE_LOG_ERROR("Vulkan surface already created");
        vkDestroySurfaceKHR(p_instance, p_surface, nullptr);
        p_surface = VK_NULL_HANDLE;
    }

    VkResult result{glfwCreateWindowSurface(p_instance, p_window, nullptr, &p_surface)};
    if (result != VK_SUCCESS) {
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                ENGINE_LOG_ERROR("Failed to create Vulkan surface. Reason: Out of host memory");
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                ENGINE_LOG_ERROR("Failed to create Vulkan surface. Reason: Out of device memory");
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                ENGINE_LOG_ERROR("Reason: Initialization failed. Check Vulkan instance and extensions");
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                ENGINE_LOG_ERROR("Failed to create Vulkan surface. Reason: Required extension not present. Make sure "
                                 "`VK_KHR_surface` and "
                                 "`VK_KHR_xcb_surface` (or equivalent) are enabled");
                break;
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                ENGINE_LOG_ERROR(
                    "Failed to create Vulkan surface. Reason: The window is already in use by another Vulkan "
                    "instance");
                break;
            default:
                ENGINE_LOG_ERROR("Failed to create Vulkan surface. Unknown Vulkan error occurred");
        }

        ENGINE_THROW("Vulkan surface creation failed");
    }

    ENGINE_LOG_INFO("Surface created");
}

} // namespace vk
} // namespace gouda