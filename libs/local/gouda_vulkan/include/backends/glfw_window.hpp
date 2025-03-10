#pragma once

#include <GLFW/glfw3.h>

#include "common.hpp"
#include "core/types.hpp"
#include "glfw_callbacks.hpp"
#include "window.hpp"

namespace Gouda {
namespace GLFW {

/**
 * @class Window
 * @brief A wrapper around GLFW for window creation, input handling, and management in the Gouda engine.
 *
 * This class provides the functionality to create a window, manage its size, handle input events, and interact
 * with the underlying GLFW library for window management. It also provides a way to set callback functions
 * for various input events and manage vsync settings.
 *
 * @tparam WindowHandle The type of the window handle (GLFWwindow* in this case).
 */
class Window : public IWindow<Callbacks<GLFWwindow *>> {
public:
    /**
     * @brief Type alias for the callback function structure specific to GLFW.
     */
    using GLFWCallbacks = Callbacks<GLFWwindow *>;

    /**
     * @brief Constructor that initializes the window based on the given configuration.
     *
     * @param config The configuration used to create the window (e.g., size, fullscreen, renderer type).
     */
    Window(const WindowConfig &config);

    /**
     * @brief Destructor that cleans up resources and destroys the GLFW window. (calls 'Destroy' internally)
     */
    ~Window() override;

    /**
     * @brief Destroys the window and cleans up associated resources.
     */
    void Destroy() override;

    /**
     * @brief Polls for events from GLFW (such as keyboard, mouse, etc.).
     *
     * This method should be called regularly to handle events like input and window resizing.
     */
    void PollEvents() override { glfwPollEvents(); };

    /**
     * @brief Checks if the window should close (e.g., due to user interaction).
     *
     * @return True if the window should close, false otherwise.
     */
    bool ShouldClose() const override { return glfwWindowShouldClose(p_window); }

    /**
     * @brief Sets the callback functions for input events such as keyboard, mouse, etc.
     *
     * The provided callback pointer should be initialized with the necessary callbacks.
     *
     * @param callbacks_ptr A pointer to the callback functions structure.
     */
    void SetCallbacks(GLFWCallbacks *callbacks_ptr) override;

    /**
     * @brief Sets the vsync (vertical synchronization) for the window.
     *
     * @param enabled A boolean flag to enable (true) or disable (false) vsync.
     */
    void SetVsync(bool enabled) override;

    /**
     * @brief Retrieves the GLFW window handle.
     *
     * @return A pointer to the GLFW window object.
     */
    GLFWwindow *GetWindow() { return p_window; }

    /**
     * @brief Retrieves the size of the window.
     *
     * The window size is returned as a structure containing width and height.
     *
     * @return The current size of the window.
     */
    WindowSize GetWindowSize() const;

    /**
     * @brief Retrieves the size of the framebuffer.
     *
     * The framebuffer size may differ from the window size, especially in cases of high DPI settings.
     *
     * @return The current framebuffer size.
     */
    FrameBufferSize GetFramebufferSize() const;

private:
    GLFWwindow *p_window;         ///< The GLFW window handle.
    WindowConfig m_window_config; ///< The configuration used to create the window.
};

} // namespace GLFW
} // namespace Gouda