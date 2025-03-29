/**
 * @file backends/glfw_backend.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-14
 * @brief Engine module implementation
 */
#include "backends/glfw/glfw_backend.hpp"

#include "imgui.h"             // Core ImGui functionality
#include "imgui_impl_glfw.h"   // GLFW backend (if using GLFW for windowing)
#include "imgui_impl_vulkan.h" // Vulkan backend for ImGui

namespace gouda {
namespace glfw {

GLFWBackend::GLFWBackend(std::function<void(Event)> event_callback) : p_callback(std::move(event_callback)) {}

void GLFWBackend::RegisterCallbacks(GLFWwindow *window)
{
    glfwSetWindowUserPointer(window, this);

    glfwSetKeyCallback(window, [](GLFWwindow *win, int key, int scancode, int action, int mods) {
#ifdef USE_IMGUI
        ImGui_ImplGlfw_KeyCallback(win, key, scancode, action, mods);
#endif
        auto *backend = static_cast<GLFWBackend *>(glfwGetWindowUserPointer(win));
        ActionState state = (action == GLFW_PRESS)     ? ActionState::Pressed
                            : (action == GLFW_RELEASE) ? ActionState::Released
                                                       : ActionState::Repeated;
        backend->p_callback(KeyEvent{Key{ConvertKey(key)}, state, mods});
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow *win, int button, int action, int mods) {
#ifdef USE_IMGUI
        ImGui_ImplGlfw_MouseButtonCallback(win, button, action, mods);
#endif

        auto *backend = static_cast<GLFWBackend *>(glfwGetWindowUserPointer(win));
        ActionState state = (action == GLFW_PRESS)     ? ActionState::Pressed
                            : (action == GLFW_RELEASE) ? ActionState::Released
                                                       : ActionState::Repeated;
        backend->p_callback(MouseButtonEvent{MouseButton{button}, state, mods});
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow *win, double x, double y) {
#ifdef USE_IMGUI
        ImGui_ImplGlfw_CursorPosCallback(win, x, y);
#endif
        auto *backend = static_cast<GLFWBackend *>(glfwGetWindowUserPointer(win));
        backend->p_callback(MouseMoveEvent{x, y});
    });

    glfwSetScrollCallback(window, [](GLFWwindow *win, double xOffset, double yOffset) {
#ifdef USE_IMGUI
        ImGui_ImplGlfw_ScrollCallback(win, xOffset, yOffset);
#endif
        auto *backend = static_cast<GLFWBackend *>(glfwGetWindowUserPointer(win));
        backend->p_callback(MouseScrollEvent{xOffset, yOffset});
    });

    glfwSetWindowCloseCallback(window, [](GLFWwindow *win) {
        auto *backend = static_cast<GLFWBackend *>(glfwGetWindowUserPointer(win));
        backend->p_callback(WindowCloseEvent{});
    });

    glfwSetCharCallback(window, [](GLFWwindow *win, unsigned int codepoint) {
#ifdef USE_IMGUI
        ImGui_ImplGlfw_CharCallback(win, codepoint);
#endif
        auto *backend = static_cast<GLFWBackend *>(glfwGetWindowUserPointer(win));
        backend->p_callback(CharEvent{codepoint});
    });

    glfwSetCursorEnterCallback(window, [](GLFWwindow *win, int entered) {
#ifdef USE_IMGUI
        ImGui_ImplGlfw_CursorEnterCallback(win, entered);
#endif
        auto *backend = static_cast<GLFWBackend *>(glfwGetWindowUserPointer(win));
        backend->p_callback(CursorEnterEvent{entered == GLFW_TRUE});
    });

    glfwSetWindowFocusCallback(window, [](GLFWwindow *win, int focused) {
#ifdef USE_IMGUI
        ImGui_ImplGlfw_WindowFocusCallback(win, focused);
#endif
        auto *backend = static_cast<GLFWBackend *>(glfwGetWindowUserPointer(win));
        backend->p_callback(WindowFocusEvent{focused == GLFW_TRUE});
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *win, int width, int height) {
        auto *backend = static_cast<GLFWBackend *>(glfwGetWindowUserPointer(win));
        backend->p_callback(WindowFramebufferSizeEvent{width, height});
    });

    glfwSetWindowSizeCallback(window, [](GLFWwindow *win, int width, int height) {
        auto *backend = static_cast<GLFWBackend *>(glfwGetWindowUserPointer(win));
        backend->p_callback(WindowSizeEvent{width, height});
    });

    glfwSetWindowIconifyCallback(window, [](GLFWwindow *win, int iconified) {
        auto *backend = static_cast<GLFWBackend *>(glfwGetWindowUserPointer(win));
        backend->p_callback(WindowIconifyEvent{iconified == GLFW_TRUE});
    });
}

void GLFWBackend::PollEvents() { glfwPollEvents(); }

Key glfw::GLFWBackend::ConvertKey(int glfw_key)
{
    static const std::unordered_map<int, Key> glfwToKeyMap = {{GLFW_KEY_SPACE, Key::Space},
                                                              {GLFW_KEY_A, Key::A},
                                                              {GLFW_KEY_B, Key::B},
                                                              {GLFW_KEY_C, Key::C},
                                                              {GLFW_KEY_D, Key::D},
                                                              {GLFW_KEY_E, Key::E},
                                                              {GLFW_KEY_F, Key::F},
                                                              {GLFW_KEY_G, Key::G},
                                                              {GLFW_KEY_H, Key::H},
                                                              {GLFW_KEY_I, Key::I},
                                                              {GLFW_KEY_J, Key::J},
                                                              {GLFW_KEY_K, Key::K},
                                                              {GLFW_KEY_L, Key::L},
                                                              {GLFW_KEY_M, Key::M},
                                                              {GLFW_KEY_N, Key::N},
                                                              {GLFW_KEY_O, Key::O},
                                                              {GLFW_KEY_P, Key::P},
                                                              {GLFW_KEY_Q, Key::Q},
                                                              {GLFW_KEY_R, Key::R},
                                                              {GLFW_KEY_S, Key::S},
                                                              {GLFW_KEY_T, Key::T},
                                                              {GLFW_KEY_U, Key::U},
                                                              {GLFW_KEY_V, Key::V},
                                                              {GLFW_KEY_W, Key::W},
                                                              {GLFW_KEY_X, Key::X},
                                                              {GLFW_KEY_Y, Key::Y},
                                                              {GLFW_KEY_Z, Key::Z},
                                                              {GLFW_KEY_ESCAPE, Key::Escape},
                                                              {GLFW_KEY_ENTER, Key::Enter},
                                                              {GLFW_KEY_TAB, Key::Tab},
                                                              {GLFW_KEY_BACKSPACE, Key::Backspace},
                                                              {GLFW_KEY_INSERT, Key::Insert},
                                                              {GLFW_KEY_DELETE, Key::Delete},
                                                              {GLFW_KEY_RIGHT, Key::Right},
                                                              {GLFW_KEY_LEFT, Key::Left},
                                                              {GLFW_KEY_DOWN, Key::Down},
                                                              {GLFW_KEY_UP, Key::Up},
                                                              {GLFW_KEY_PAGE_UP, Key::PageUp},
                                                              {GLFW_KEY_PAGE_DOWN, Key::PageDown},
                                                              {GLFW_KEY_HOME, Key::Home},
                                                              {GLFW_KEY_END, Key::End},
                                                              {GLFW_KEY_CAPS_LOCK, Key::CapsLock},
                                                              {GLFW_KEY_SCROLL_LOCK, Key::ScrollLock},
                                                              {GLFW_KEY_NUM_LOCK, Key::NumLock},
                                                              {GLFW_KEY_PRINT_SCREEN, Key::PrintScreen},
                                                              {GLFW_KEY_PAUSE, Key::Pause},
                                                              {GLFW_KEY_F1, Key::F1},
                                                              {GLFW_KEY_F2, Key::F2},
                                                              {GLFW_KEY_F3, Key::F3},
                                                              {GLFW_KEY_F4, Key::F4},
                                                              {GLFW_KEY_F5, Key::F5},
                                                              {GLFW_KEY_F6, Key::F6},
                                                              {GLFW_KEY_F7, Key::F7},
                                                              {GLFW_KEY_F8, Key::F8},
                                                              {GLFW_KEY_F9, Key::F9},
                                                              {GLFW_KEY_F10, Key::F10},
                                                              {GLFW_KEY_F11, Key::F11},
                                                              {GLFW_KEY_F12, Key::F12},
                                                              {GLFW_KEY_KP_0, Key::KP0},
                                                              {GLFW_KEY_KP_1, Key::KP1},
                                                              {GLFW_KEY_KP_2, Key::KP2},
                                                              {GLFW_KEY_KP_3, Key::KP3},
                                                              {GLFW_KEY_KP_4, Key::KP4},
                                                              {GLFW_KEY_KP_5, Key::KP5},
                                                              {GLFW_KEY_KP_6, Key::KP6},
                                                              {GLFW_KEY_KP_7, Key::KP7},
                                                              {GLFW_KEY_KP_8, Key::KP8},
                                                              {GLFW_KEY_KP_9, Key::KP9},
                                                              {GLFW_KEY_LEFT_SHIFT, Key::LeftShift},
                                                              {GLFW_KEY_RIGHT_SHIFT, Key::RightShift},
                                                              {GLFW_KEY_LEFT_CONTROL, Key::LeftControl},
                                                              {GLFW_KEY_RIGHT_CONTROL, Key::RightControl},
                                                              {GLFW_KEY_LEFT_ALT, Key::LeftAlt},
                                                              {GLFW_KEY_RIGHT_ALT, Key::RightAlt},
                                                              {GLFW_KEY_MENU, Key::Menu},
                                                              {GLFW_KEY_UNKNOWN, Key::Unknown}};

    auto it = glfwToKeyMap.find(glfw_key);
    return (it != glfwToKeyMap.end()) ? it->second : Key::None;
}

} // namespace glfw
} // namespace gouda