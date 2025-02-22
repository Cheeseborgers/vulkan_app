#include "gouda_vk_glfw.hpp"

#include "logger.hpp"

#include "gouda_throw.hpp"

namespace GoudaVK {
namespace GLFW {

// ---- INTERNAL HELPER FUNCTIONS ----
namespace internal {

// GLFW Error callback (only used inside this file)
inline void ErrorCallback(int error, const char *description)
{
    ENGINE_LOG_ERROR("GLFW error ({}): {}", error, description);
}

// Get platform name as string (only used internally)
inline std::string_view get_platform_as_string(Platform platform)
{
    switch (platform) {
        case Platform::X11:
            return "X11";
        case Platform::Wayland:
            return "Wayland";
        case Platform::Windows:
            return "Windows";
        case Platform::Headless:
            return "Headless";
        case Platform::SystemDefault:
            return "System default";
        default:
            return "Unknown";
    }
}

inline void print_initialized_glfw_system_platform()
{
    // Verify the selected platform
    int platform{glfwGetPlatform()};
    switch (platform) {
        case GLFW_PLATFORM_WAYLAND:
            ENGINE_LOG_INFO("Using Wayland platform");
            break;
        case GLFW_PLATFORM_X11:
            ENGINE_LOG_INFO("Using X11 platform");
            break;
        case GLFW_PLATFORM_WIN32:
            ENGINE_LOG_INFO("Using Windows platform");
            break;
        case GLFW_PLATFORM_COCOA:
            ENGINE_LOG_INFO("Using MacOS platform");
            break;
        case GLFW_PLATFORM_NULL:
            ENGINE_LOG_WARNING("Running in headless mode (GLFW_PLATFORM_NULL)");
            break;
        default:
            ENGINE_LOG_FATAL("Unknown GLFW platform!");
            glfwTerminate();
            ENGINE_THROW("Unknown GLFW platform");
    }
}

} // namespace internal

// ---- PUBLIC FUNCTIONS ----

// Set the platform before initializing GLFW
void set_platform(Platform platform)
{
    switch (platform) {
        case Platform::X11:
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
            break;
        case Platform::Wayland:
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
            break;
        case Platform::Windows:
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WIN32);
            break;
        case Platform::Headless:
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_NULL);
            break;
        case Platform::SystemDefault:
            break; // No forced platform
    }
}

// Initialize Vulkan-compatible GLFW window
std::optional<GLFWwindow *> vulkan_init(WindowSize window_size, std::string_view application_title,
                                        bool window_resizable, Platform platform)
{
    glfwSetErrorCallback(internal::ErrorCallback);

    // Set the chosen platform
    set_platform(platform);

    if (!glfwInit()) {
        ENGINE_THROW("Failed to initialize GLFW using {}", internal::get_platform_as_string(platform));
        return std::nullopt;
    }

    internal::print_initialized_glfw_system_platform();

    if (!glfwVulkanSupported()) {
        ENGINE_THROW("Failed to find a Vulkan loader and an ICD");
        return std::nullopt;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, window_resizable);

    GLFWwindow *window_ptr{
        glfwCreateWindow(window_size.width, window_size.height, application_title.data(), nullptr, nullptr)};

    if (!window_ptr) {
        const char *error_description{nullptr};
        int error_code{glfwGetError(&error_description)};

        ENGINE_LOG_FATAL("Failed to create GLFW window. Error code: {}", error_code);
        if (error_description) {
            ENGINE_LOG_FATAL("Error description: {}", error_description);
        }

        glfwTerminate();
        return std::nullopt;
    }

    return window_ptr;
}

// Set GLFW event callbacks
void set_callbacks(GLFWwindow *window_ptr, GLFW::Callbacks *callbacks_ptr)
{
    glfwSetWindowUserPointer(window_ptr, callbacks_ptr); // Store the callback struct in GLFW

    glfwSetKeyCallback(window_ptr, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        auto *cb = static_cast<GLFW::Callbacks *>(glfwGetWindowUserPointer(window));
        if (cb && cb->key_callback)
            cb->key_callback(window, key, scancode, action, mods);
    });

    glfwSetCursorPosCallback(window_ptr, [](GLFWwindow *window, double xpos, double ypos) {
        auto *cb = static_cast<GLFW::Callbacks *>(glfwGetWindowUserPointer(window));
        if (cb && cb->mouse_move_callback)
            cb->mouse_move_callback(window, static_cast<f32>(xpos), static_cast<f32>(ypos));
    });

    glfwSetMouseButtonCallback(window_ptr, [](GLFWwindow *window, int button, int action, int mods) {
        auto *cb = static_cast<GLFW::Callbacks *>(glfwGetWindowUserPointer(window));
        if (cb && cb->mouse_button_callback)
            cb->mouse_button_callback(window, button, action, mods);
    });

    glfwSetScrollCallback(window_ptr, [](GLFWwindow *window, double x_offset, double y_offset) {
        auto *cb = static_cast<GLFW::Callbacks *>(glfwGetWindowUserPointer(window));
        if (cb && cb->mouse_scroll_callback)
            cb->mouse_scroll_callback(window, static_cast<f32>(x_offset), static_cast<f32>(y_offset));
    });

    glfwSetFramebufferSizeCallback(window_ptr, [](GLFWwindow *window, int width, int height) {
        auto *cb = static_cast<GLFW::Callbacks *>(glfwGetWindowUserPointer(window));
        if (cb && cb->framebuffer_resized_callback)
            cb->framebuffer_resized_callback(window, FrameBufferSize{width, height});
    });

    glfwSetWindowIconifyCallback(window_ptr, [](GLFWwindow *window, int iconified) {
        auto *cb = static_cast<GLFW::Callbacks *>(glfwGetWindowUserPointer(window));
        if (cb && cb->window_iconify_callback)
            cb->window_iconify_callback(window, iconified);
    });
}

// Set the GLFW window icon
void set_window_icon(GLFWwindow *window, std::string_view file_path)
{
    // StbiImage image = StbiImage(file_path);

    // if (!image.GetData()) {
    //    printf("Failed to load icon\n");
    //    return;
    //}

    // GLFWimage icon;
    // icon.width = image.GetWidth();
    // icon.height = image.GetHeight();
    // icon.pixels = image.GetData();

    // glfwSetWindowIcon(window, 1, &icon); // Apply icon to window
}

} // namespace GLFW
} // namespace GoudaVK
