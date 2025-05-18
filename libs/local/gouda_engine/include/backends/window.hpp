#pragma once

namespace gouda {

/**
 * @class IWindow
 * @brief Abstract base class for window management in the Gouda engine.
 *
 * This class provides an interface for creating and managing windows, including handling
 * events (input, resizing, etc.), controlling vsync, and window destruction.
 * It serves as a base class for platform-specific implementations (e.g., GLFW, SDL).
 *
 * @tparam CallbackType The type of the callback functions used for window events.
 */
class IWindow {
public:
    /**
     * @brief Virtual destructor for IWindow, ensures proper cleanup in derived classes.
     */
    virtual ~IWindow() = default;

    /**
     * @brief Destroys the window and releases any associated resources.
     *
     * This method must be implemented by derived classes to handle window-specific cleanup.
     */
    virtual void Destroy() = 0;

    /**
     * @brief Polls for window events such as keyboard, mouse, and window events.
     *
     * This method must be implemented by derived classes to handle platform-specific event polling.
     */
    virtual void PollEvents() = 0;

    /**
     * @brief Checks whether the window should be closed (e.g., user triggered close).
     *
     * @return True if the window should close, otherwise false.
     */
    [[nodiscard]] virtual bool ShouldClose() const = 0;

    /**
     * @brief Sets the vertical synchronization (vsync) for the window.
     *
     * @param enabled A boolean flag indicating whether to enable (true) or disable (false) vsync.
     */
    virtual void SetVsync(bool enabled) = 0;

    /**
     * @brief Sets the icon for the window.
     *
     * @param filepath The file path for the window decoration icon image
     */
    virtual void SetIcon(std::string_view filepath) = 0;

    /**
     * @brief Loads the mouse cursor for the window.
     *
     * @param filepath The file path for the mouse cursor image
     * @param identifier The identifier for the mouse cursor
     */
    virtual void LoadCursor(std::string_view filepath, std::string_view identifier) = 0;

    /**
     * @brief Sets the mouse cursor for the window.
     *
     * @param identifier The identifier for the loaded cursor
     */
    virtual void SetCursor(std::string_view identifier) = 0;

    /**
     * @brief Destroys any stored and loaded mouse cursors.
     *
     */
    virtual void ClearCursors() = 0;
};

} // namespace gouda