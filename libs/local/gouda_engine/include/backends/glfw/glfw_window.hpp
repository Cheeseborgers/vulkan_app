#pragma once

#include <GLFW/glfw3.h>

#include "backends/common.hpp"
#include "backends/window.hpp"
#include "core/types.hpp"


namespace gouda::glfw {

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
class Window final : public IWindow {
public:
    /**
     * @brief Constructor that initializes the window based on the given configuration.
     *
     * @param config The configuration used to create the window (e.g., size, fullscreen, renderer type).
     */
    explicit Window(const WindowConfig &config);

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
     * @brief Closes the current window, closing the application.
     *
     */
    void Close() const override { glfwSetWindowShouldClose(p_window, GLFW_TRUE); }

    /**
     * @brief Sets the vsync (vertical synchronization) for the window.
     *
     * @param enabled A boolean flag to enable (true) or disable (false) vsync.
     */
    void SetVsync(bool enabled) override;

    /**
     * @brief Sets the icon for the window.
     *
     * @param filepath The file path for the window decoration icon
     */
    void SetIcon(StringView filepath) override;

    /**
     * @brief Loads the mouse cursor for the window.
     *
     * @param filepath The file path for the mouse cursor image
     * @param identifier The identifier for the mouse cursor
     */
    void LoadCursor(StringView filepath, StringView identifier) override;

    /**
     * @brief Sets the mouse cursor for the window.
     *
     * @param filepath The identifier for the loaded cursor
     */
    void SetCursor(StringView filepath) override;

    /**
     * @brief Destroys any stored and loaded mouse cursors.
     *
     */
    void ClearCursors() override;

    /**
     * @brief Retrieves the GLFW window handle.
     *
     * @return A pointer to the GLFW window object.
     */
    GLFWwindow *GetWindow() const { return p_window; }

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
    GLFWwindow *p_window; ///< The GLFW window handle.
    Renderer m_renderer;  ///< The Renderer type. // TODO: Do we need the render? why not use the type from config?

    std::unordered_map<std::string, GLFWcursor *> m_cursors; ///< Current loaded cursors
};

} // namespace gouda::glfw
