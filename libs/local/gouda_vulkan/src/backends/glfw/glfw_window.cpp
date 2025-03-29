#include "backends/glfw/glfw_window.hpp"

#include "debug/debug.hpp"
#include "utils/image.hpp"

namespace gouda {
namespace glfw {

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
        {GLFW_PLATFORM_WAYLAND, internal::constants::Wayland},
        {GLFW_PLATFORM_X11, internal::constants::X11},
        {GLFW_PLATFORM_WIN32, internal::constants::Windows},
        {GLFW_PLATFORM_COCOA, internal::constants::MacOS},
        {GLFW_PLATFORM_NULL, internal::constants::Headless}};

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

Window::Window(const WindowConfig &config) : p_window{nullptr}
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
        ClearCursors();

        glfwDestroyWindow(p_window);
        p_window = nullptr;
        ENGINE_LOG_DEBUG("Window Destroyed");
    }

    glfwTerminate();
}

void Window::SetVsync(bool enabled)
{
    if (m_renderer == Renderer::OpenGL) {
        if (enabled) {
            glfwSwapInterval(1); // Enable vsync for OpenGL
        }
        else {
            glfwSwapInterval(0); // Disable vsync for OpenGL
        }
    }
    else if (m_renderer == Renderer::Vulkan) {
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

void Window::SetIcon(std::string_view filepath)
{
    auto image_result = Image::Load(filepath);
    if (!image_result) {
        ENGINE_LOG_ERROR("Could not load window icon image: {}", image_result.error());
        return;
    }

    Image original = std::move(image_result.value());

    // Resize to 32x32
    auto small_icon_result = original.Resize(32, 32);
    if (!small_icon_result) {
        ENGINE_LOG_ERROR("Failed to resize window icon to 32x32: {}", small_icon_result.error());
        return;
    }

    Image small_icon = std::move(small_icon_result.value());
    GLFWimage icons[2];

    // 64x64 original image
    icons[0].width = original.GetWidth();
    icons[0].height = original.GetHeight();
    icons[0].pixels = const_cast<stbi_uc *>(original.data().data());

    // 32x32 resized image
    icons[1].width = small_icon.GetWidth();
    icons[1].height = small_icon.GetHeight();
    icons[1].pixels = const_cast<stbi_uc *>(small_icon.data().data());

    // Set the window icons
    glfwSetWindowIcon(p_window, 2, icons);

    ENGINE_LOG_INFO("Window icon set successfully.");
}

void Window::LoadCursor(std::string_view filepath, std::string_view identifier)
{
    auto image_result = Image::Load(filepath);
    if (!image_result) {
        ENGINE_LOG_ERROR("Could not load mouse cursor image: {}", image_result.error());
        return;
    }

    Image image{std::move(image_result.value())};
    GLFWimage cursor_image{};
    cursor_image.width = image.GetWidth();
    cursor_image.height = image.GetHeight();
    cursor_image.pixels = const_cast<stbi_uc *>(image.data().data());

    // Create the GLFW cursor
    GLFWcursor *cursor{glfwCreateCursor(&cursor_image, 0, 0)};
    if (!cursor) {
        ENGINE_LOG_ERROR("Failed to create cursor from image: {}", filepath);
        return;
    }

    m_cursors.emplace(std::string(identifier), cursor);
}

void Window::SetCursor(std::string_view identifier)
{
    ASSERT(p_window, "Window is not initialized or invalid!");

    if (!p_window) {
        ENGINE_LOG_ERROR("Window is not initialized, cannot set cursor.");
        return;
    }

    // Find the cursor by its identifier in the map
    auto it = m_cursors.find(std::string(identifier));
    if (it != m_cursors.end() && it->second) {
        // Set the cursor if it exists
        glfwSetCursor(p_window, it->second);
    }
    else {
        ENGINE_LOG_ERROR("Cursor '{}' not found.", identifier);
    }
}

void Window::ClearCursors()
{
    if (p_window) {
        for (auto &[key, cursor] : m_cursors) {
            glfwDestroyCursor(static_cast<GLFWcursor *>(cursor));
        }
        m_cursors.clear();

        glfwSetCursor(p_window, nullptr);

        ENGINE_LOG_DEBUG("GLFW cursors cleared.");
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

} // namespace glfw
} // namespace gouda