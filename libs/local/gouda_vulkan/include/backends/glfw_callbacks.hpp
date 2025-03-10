#pragma once

#include <GLFW/glfw3.h>

#include "core/types.hpp"

namespace Gouda {
namespace GLFW {

template <typename WindowHandle>
struct Callbacks {
    using KeyCallback = std::function<void(WindowHandle, int, int, int, int)>;
    using MouseMoveCallback = std::function<void(WindowHandle, double, double)>;
    using MouseButtonCallback = std::function<void(WindowHandle, int, int, int)>;
    using MouseScrollCallback = std::function<void(WindowHandle, double, double)>;
    using WindowIconifyCallback = std::function<void(WindowHandle, int)>;
    using FramebufferResizedCallback = std::function<void(WindowHandle, FrameBufferSize)>;

    KeyCallback key_callback;
    MouseMoveCallback mouse_move_callback;
    MouseButtonCallback mouse_button_callback;
    MouseScrollCallback mouse_scroll_callback;
    WindowIconifyCallback window_iconify_callback;
    FramebufferResizedCallback framebuffer_resized_callback;

    static void KeyCallbackTrampoline(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        auto callbacks = static_cast<Callbacks *>(glfwGetWindowUserPointer(window));
        if (callbacks && callbacks->key_callback) {
            callbacks->key_callback(window, key, scancode, action, mods);
        }
    }

    static void MouseMoveCallbackTrampoline(GLFWwindow *window, double xpos, double ypos)
    {
        auto callbacks = static_cast<Callbacks *>(glfwGetWindowUserPointer(window));
        if (callbacks && callbacks->mouse_move_callback) {
            callbacks->mouse_move_callback(window, xpos, ypos);
        }
    }

    static void MouseButtonCallbackTrampoline(GLFWwindow *window, int button, int action, int mods)
    {
        auto callbacks = static_cast<Callbacks *>(glfwGetWindowUserPointer(window));
        if (callbacks && callbacks->mouse_button_callback) {
            callbacks->mouse_button_callback(window, button, action, mods);
        }
    }

    static void MouseScrollCallbackTrampoline(GLFWwindow *window, double xoffset, double yoffset)
    {
        auto callbacks = static_cast<Callbacks *>(glfwGetWindowUserPointer(window));
        if (callbacks && callbacks->mouse_scroll_callback) {
            callbacks->mouse_scroll_callback(window, xoffset, yoffset);
        }
    }

    static void WindowIconifyCallbackTrampoline(GLFWwindow *window, int iconified)
    {
        auto callbacks = static_cast<Callbacks *>(glfwGetWindowUserPointer(window));
        if (callbacks && callbacks->window_iconify_callback) {
            callbacks->window_iconify_callback(window, iconified);
        }
    }

    static void FramebufferResizedCallbackTrampoline(GLFWwindow *window, int width, int height)
    {
        auto callbacks = static_cast<Callbacks *>(glfwGetWindowUserPointer(window));
        if (callbacks && callbacks->framebuffer_resized_callback) {
            callbacks->framebuffer_resized_callback(window, {width, height});
        }
    }
};

} // namespace GLFW end
} // namespace Gouda end