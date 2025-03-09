#include "backends/glfw_window.hpp"

#include "gouda_throw.hpp"
#include "logger.hpp"

namespace Gouda {
namespace GLFW {

// GLFW Error callback
static void error_callback(int error, const char *description)
{
    ENGINE_LOG_ERROR("GLFW error ({}): {}", error, description);
}

// Detect GLFW Platform
static std::optional<std::string_view> detect_glfw_platform()
{
    int platform{glfwGetPlatform()};

    // Using an unordered_map to map platform codes to platform strings
    static const std::unordered_map<int, std::string_view> platform_map{
        {GLFW_PLATFORM_WAYLAND, Internal::Constants::Wayland},
        {GLFW_PLATFORM_X11, Internal::Constants::X11},
        {GLFW_PLATFORM_WIN32, Internal::Constants::Windows},
        {GLFW_PLATFORM_COCOA, Internal::Constants::MacOS},
        {GLFW_PLATFORM_NULL, Internal::Constants::Headless}};

    if (platform_map.find(platform) != platform_map.end()) {
        return platform_map.at(platform); // Return the mapped platform string
    }

    return std::nullopt; // Return nullopt if the platform is unknown
}

// Log GLFW platform info
static void log_glfw_platform_info()
{
    auto platform = detect_glfw_platform();

    if (!platform) {
        ENGINE_LOG_FATAL("Unknown GLFW platform! Terminating application");
        glfwTerminate();
    }
    else {
        ENGINE_LOG_INFO("Using {} platform", *platform);
    }
}

// Set GLFW platform hints
static void set_platform_hints(Platform platform)
{
    static const std::unordered_map<Platform, int> platform_hints{{Platform::X11, GLFW_PLATFORM_X11},
                                                                  {Platform::Wayland, GLFW_PLATFORM_WAYLAND},
                                                                  {Platform::Windows, GLFW_PLATFORM_WIN32},
                                                                  {Platform::Headless, GLFW_PLATFORM_NULL}};

    if (platform_hints.contains(platform)) {
        glfwInitHint(GLFW_PLATFORM, platform_hints.at(platform));
    }
}

Window::Window(const WindowConfig &config) : p_window{nullptr}, m_window_config{config}
{
    if (config.size.area() == 0) {
        ENGINE_THROW("Failed to initialize GLFW as window dimensions cannot be zero. Size: {}x{}", config.size.width,
                     config.size.height);
    }

    glfwSetErrorCallback(error_callback);

    set_platform_hints(config.platform);

    if (!glfwInit()) {
        ENGINE_THROW("Failed to initialize GLFW using {}", platform_to_string(config.platform));
    }

    log_glfw_platform_info();

    // Handle renderer-specific hints and checks
    switch (config.renderer) {
        case Renderer::Vulkan:
            if (!glfwVulkanSupported()) {
                ENGINE_LOG_FATAL("Failed to find a Vulkan loader and an ICD. Terminating application");
                Destroy();
            }
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            ENGINE_LOG_INFO("GLFW set to use Vulkan renderer (GLFW_NO_API)");
            break;

        case Renderer::OpenGL:
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
            ENGINE_LOG_INFO("GLFW set to use OpenGL renderer (GLFW_OPENGL_API)");
            break;

        default:
            ENGINE_LOG_FATAL("Unknown renderer type specified. Terminating application");
            Destroy();
            break;
    }

    // Set window resizable
    glfwWindowHint(GLFW_RESIZABLE, config.resizable);

    // Determine monitor for fullscreen mode
    GLFWmonitor *monitor{nullptr};

    if (config.fullscreen) {
        monitor = glfwGetPrimaryMonitor(); // Get the primary monitor for fullscreen
        if (!monitor) {
            ENGINE_LOG_FATAL("Failed to retrieve primary monitor for fullscreen mode.");
            Destroy();
        }

        const GLFWvidmode *mode{glfwGetVideoMode(monitor)}; // Get the monitor's current resolution & refresh rate
        if (!mode) {
            ENGINE_LOG_FATAL("Failed to retrieve video mode for primary monitor.");
            Destroy();
        }

        // Only set refresh rate if not using Vulkan, and user hasn't explicitly disabled vsync.
        if (config.renderer != Renderer::Vulkan) {
            if (config.refresh_rate != 0 && config.vsync == false) {
                glfwWindowHint(GLFW_REFRESH_RATE, config.refresh_rate);
            }
            else {
                glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate); // Default to monitor's refresh rate
            }
        }

        // Create a fullscreen window at the monitor's native resolution
        p_window = glfwCreateWindow(mode->width, mode->height, config.title.data(), monitor, nullptr);
    }
    else {
        // Create a windowed mode window with user-specified size
        p_window = glfwCreateWindow(config.size.width, config.size.height, config.title.data(), nullptr, nullptr);
    }

    // Error handling
    if (!p_window) {
        const char *error_description{nullptr};
        int error_code = glfwGetError(&error_description);

        ENGINE_LOG_FATAL("Failed to create GLFW window. Error code: {}", error_code);
        if (error_description) {
            ENGINE_LOG_FATAL("Error description: {}", error_description);
        }

        Destroy();
    }
}

Window::~Window() { Destroy(); }

void Window::Destroy()
{
    if (p_window) {
        glfwDestroyWindow(p_window);
        p_window = nullptr;
    }

    ENGINE_LOG_DEBUG("Window Destroyed");

    glfwTerminate();
}

void Window::SetCallbacks(GLFWCallbacks *callbacks_ptr)
{
    // TODO: Create event system and move all of these there
    if (!p_window || !callbacks_ptr)
        return;

    // Store the callbacks in GLFW's user pointer storage
    glfwSetWindowUserPointer(p_window, callbacks_ptr);

    // Set the callback functions using the trampoline method
    glfwSetKeyCallback(p_window, &GLFWCallbacks::KeyCallbackTrampoline);
    glfwSetCursorPosCallback(p_window, &GLFWCallbacks::MouseMoveCallbackTrampoline);
    glfwSetMouseButtonCallback(p_window, &GLFWCallbacks::MouseButtonCallbackTrampoline);
    glfwSetScrollCallback(p_window, &GLFWCallbacks::MouseScrollCallbackTrampoline);
    glfwSetWindowIconifyCallback(p_window, &GLFWCallbacks::WindowIconifyCallbackTrampoline);
    glfwSetFramebufferSizeCallback(p_window, &GLFWCallbacks::FramebufferResizedCallbackTrampoline);
}

void Window::SetVsync(bool enabled)
{
    if (m_window_config.renderer == Renderer::OpenGL) {
        if (enabled) {
            glfwSwapInterval(1); // Enable vsync for OpenGL
        }
        else {
            glfwSwapInterval(0); // Disable vsync for OpenGL
        }
    }
    else if (m_window_config.renderer == Renderer::Vulkan) {
        // For Vulkan, you'd typically need to adjust the swapchain's present mode.
        // You would need to recreate the swapchain if you're changing vsync settings in Vulkan.

        if (enabled) {
            // Change Vulkan swapchain to use vsync (FIFO present mode or similar)
            // You'll need Vulkan-specific code to recreate the swapchain with vsync enabled
            ENGINE_LOG_INFO("Vulkan: Vsync enabled");
        }
        else {
            // Change Vulkan swapchain to disable vsync
            // You would set a present mode that doesn't use vsync (e.g., immediate present mode)
            ENGINE_LOG_INFO("Vulkan: Vsync disabled");
        }

        // Note: You will likely need to recreate the swapchain when changing the present mode.
    }
    else {
        ENGINE_LOG_WARNING("Renderer not supported for vsync toggle.");
    }
}

WindowSize Window::GetWindowSize() const
{
    WindowSize size{0, 0};
    glfwGetWindowSize(p_window, &size.width, &size.height);
    return size;
}

FrameBufferSize Window::GetFramebufferSize() const
{
    FrameBufferSize size{0, 0};
    glfwGetFramebufferSize(p_window, &size.width, &size.height);
    return size;
}

} // namespace GLFW
} // namespace Gouda