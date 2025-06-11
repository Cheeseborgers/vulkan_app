/**
 * @file input_handler.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-14
 * @brief Engine module implementation
 */

#include "backends/input_handler.hpp"

#include "debug/logger.hpp"

// TODO: Remove this!
#include <GLFW/glfw3.h>

namespace gouda {

InputHandler::InputHandler(std::unique_ptr<InputBackend> backend, GLFWwindow *window)
    : p_backend(std::move(backend)),
      p_window(window),
      m_scroll_callback(nullptr),
      m_char_callback(nullptr),
      m_cursor_enter_callback(nullptr),
      m_window_focus_callback(nullptr),
      m_frame_buffer_size_callback(nullptr),
      m_window_size_callback(nullptr),
      m_window_iconify_callback(nullptr),
      m_mouse_x(0),
      m_mouse_y(0)
{
    p_backend->RegisterCallbacks(p_window);
}

void InputHandler::LoadStateBindings(const std::string &state, const std::vector<ActionBinding> &bindings)
{
    m_bindings.erase(state);
    m_bindings.emplace(state, bindings);
    ENGINE_LOG_DEBUG("Loaded {} bindings for state '{}'", bindings.size(), state);
}

void InputHandler::UnloadStateBindings(const std::string &state)
{
    m_bindings.erase(state);
    ENGINE_LOG_DEBUG("Unloaded bindings for state '{}'", state);
}

void InputHandler::SetActiveState(const std::string &state)
{
    m_active_state = state;
    ApplyStateCallbacks(state); // Apply state-specific callbacks when switching states
    ENGINE_LOG_DEBUG("Set active state to '{}'", state);
}

void InputHandler::PushCustomEvent(const std::string &event) { m_events.push_back(event); }

void InputHandler::QueueEvent(Event event) { m_events.push_back(event); }

// Standard callback setters
void InputHandler::SetScrollCallback(ScrollCallback callback) { m_scroll_callback = std::move(callback); }

void InputHandler::SetCharCallback(CharCallback callback) { m_char_callback = std::move(callback); }

void InputHandler::SetCursorEnterCallback(CursorEnterCallback callback)
{
    m_cursor_enter_callback = std::move(callback);
}

void InputHandler::SetWindowFocusCallback(WindowFocusCallback callback)
{
    m_window_focus_callback = std::move(callback);
}

void InputHandler::SetFramebufferSizeCallback(FramebufferSizeCallback callback)
{
    m_frame_buffer_size_callback = std::move(callback);
}

void InputHandler::SetWindowSizeCallback(WindowSizeCallback callback) { m_window_size_callback = std::move(callback); }

void InputHandler::SetWindowIconifyCallback(WindowIconifyCallback callback)
{
    m_window_iconify_callback = std::move(callback);
}

// State-specific callback setters
void InputHandler::SetStateScrollCallback(const std::string &state, ScrollCallback callback)
{
    m_state_scroll_callbacks[state] = std::move(callback);
    if (m_active_state == state) {
        m_scroll_callback = m_state_scroll_callbacks[state];
    }
}

void InputHandler::SetStateCharCallback(const std::string &state, CharCallback callback)
{
    m_state_char_callbacks[state] = std::move(callback);
    if (m_active_state == state) {
        m_char_callback = m_state_char_callbacks[state];
    }
}

void InputHandler::SetStateCursorEnterCallback(const std::string &state, CursorEnterCallback callback)
{
    m_state_cursor_enter_callbacks[state] = std::move(callback);
    if (m_active_state == state) {
        m_cursor_enter_callback = m_state_cursor_enter_callbacks[state];
    }
}

void InputHandler::SetStateWindowFocusCallback(const std::string &state, WindowFocusCallback callback)
{
    m_state_window_focus_callbacks[state] = std::move(callback);
    if (m_active_state == state) {
        m_window_focus_callback = m_state_window_focus_callbacks[state];
    }
}

void InputHandler::SetStateFramebufferSizeCallback(const std::string &state, FramebufferSizeCallback callback)
{
    m_state_framebuffer_size_callbacks[state] = std::move(callback);
    if (m_active_state == state) {
        m_frame_buffer_size_callback = m_state_framebuffer_size_callbacks[state];
    }
}

void InputHandler::SetStateWindowSizeCallback(const std::string &state, WindowSizeCallback callback)
{
    m_state_window_size_callbacks[state] = std::move(callback);
    if (m_active_state == state) {
        m_window_size_callback = m_state_window_size_callbacks[state];
    }
}

void InputHandler::SetStateWindowIconifyCallback(const std::string &state, WindowIconifyCallback callback)
{
    m_state_window_iconify_callbacks[state] = std::move(callback);
    if (m_active_state == state) {
        m_window_iconify_callback = m_state_window_iconify_callbacks[state];
    }
}

// Clear callback methods
void InputHandler::ClearScrollCallback()
{
    m_scroll_callback = nullptr;
    ENGINE_LOG_DEBUG("Scroll callback cleared");
}

void InputHandler::ClearCharCallback()
{
    m_char_callback = nullptr;
    ENGINE_LOG_DEBUG("Char callback cleared");
}

void InputHandler::ClearCursorEnterCallback()
{
    m_cursor_enter_callback = nullptr;
    ENGINE_LOG_DEBUG("Cursor enter callback cleared");
}

void InputHandler::ClearWindowFocusCallback()
{
    m_window_focus_callback = nullptr;
    ENGINE_LOG_DEBUG("Window focus callback cleared");
}

void InputHandler::ClearFramebufferSizeCallback()
{
    m_frame_buffer_size_callback = nullptr;
    ENGINE_LOG_DEBUG("Framebuffer size callback cleared");
}

void InputHandler::ClearWindowSizeCallback()
{
    m_window_size_callback = nullptr;
    ENGINE_LOG_DEBUG("Window size callback cleared");
}

void InputHandler::ClearWindowIconifyCallback()
{
    m_window_iconify_callback = nullptr;
    ENGINE_LOG_DEBUG("Window iconify callback cleared");
}

bool InputHandler::IsKeyPressed(const Key key) const
{
    const auto it = m_key_states.find(key);
    return it != m_key_states.end() ? it->second : false;
}

bool InputHandler::IsMouseButtonPressed(const MouseButton button) const
{
    const auto it = m_mouse_states.find(button);
    return it != m_mouse_states.end() ? it->second : false;
}

std::pair<double, double> InputHandler::GetMousePosition() const { return {m_mouse_x, m_mouse_y}; }

void InputHandler::Update()
{
    p_backend->PollEvents();
    ProcessEvents();
}

void InputHandler::ProcessEvents()
{
    for (auto &event : m_events) {
        std::visit(
            [this](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, KeyEvent>) {
                    // ENGINE_LOG_DEBUG("Processing KeyEvent: key={}, state={}", static_cast<int>(arg.key),
                    //                 static_cast<int>(arg.state));
                    m_key_states[arg.key] = (arg.state != ActionState::Released);
                    if (auto it = m_bindings.find(m_active_state); it != m_bindings.end()) {
                        for (const auto &binding : it->second) {
                            if (auto *key = std::get_if<Key>(&binding.input)) {
                                if (*key == arg.key && binding.trigger_state == arg.state) {
                                    // ENGINE_LOG_DEBUG("Triggering Key binding: key={}", static_cast<int>(*key));
                                    binding.callback();
                                }
                            }
                        }
                    }
                }
                else if constexpr (std::is_same_v<T, MouseButtonEvent>) {
                    // ENGINE_LOG_DEBUG("Processing MouseButtonEvent: button={}, state={}",
                    // static_cast<int>(arg.button),
                    //                  static_cast<int>(arg.state));
                    m_mouse_states[arg.button] = (arg.state != ActionState::Released);
                    if (auto it = m_bindings.find(m_active_state); it != m_bindings.end()) {
                        for (const auto &binding : it->second) {
                            if (auto *button = std::get_if<MouseButton>(&binding.input)) {
                                if (*button == arg.button && binding.trigger_state == arg.state) {
                                    // ENGINE_LOG_DEBUG("Triggering Mouse binding: button={}",
                                    // static_cast<int>(*button));
                                    binding.callback();
                                }
                            }
                        }
                    }
                }
                else if constexpr (std::is_same_v<T, MouseScrollEvent>) {
                    // APP_LOG_DEBUG("Processing MouseScrollEvent: xOffset={}, yOffset={}", arg.xOffset, arg.yOffset);
                    if (m_scroll_callback) {
                        m_scroll_callback(arg.xOffset, arg.yOffset);
                    }
                }
                else if constexpr (std::is_same_v<T, MouseMoveEvent>) {
                    m_mouse_x = arg.x; // Update mouse position
                    m_mouse_y = arg.y;
                }
                else if constexpr (std::is_same_v<T, CharEvent>) {
                    // APP_LOG_DEBUG("Processing CharEvent: codepoint={}", arg.codepoint);
                    if (m_char_callback) {
                        m_char_callback(arg.codepoint);
                    }
                }
                else if constexpr (std::is_same_v<T, CursorEnterEvent>) {
                    // APP_LOG_DEBUG("Processing CursorEnterEvent: entered={}", arg.entered);
                    if (m_cursor_enter_callback) {
                        m_cursor_enter_callback(arg.entered);
                    }
                }
                else if constexpr (std::is_same_v<T, WindowFocusEvent>) {
                    // APP_LOG_DEBUG("Processing WindowFocusEvent: focused={}", arg.focused);
                    if (m_window_focus_callback) {
                        m_window_focus_callback(arg.focused);
                    }
                }
                else if constexpr (std::is_same_v<T, WindowFramebufferSizeEvent>) {
                    // APP_LOG_DEBUG("Processing WindowFramebufferSizeEvent: width={}, height={}", arg.width,
                    // arg.height);
                    if (m_frame_buffer_size_callback) {
                        m_frame_buffer_size_callback(arg.width, arg.height);
                    }
                }
                else if constexpr (std::is_same_v<T, WindowSizeEvent>) {
                    // APP_LOG_DEBUG("Processing WindowSizeEvent: width={}, height={}", arg.width, arg.height);
                    if (m_window_size_callback) {
                        m_window_size_callback(arg.width, arg.height);
                    }
                }
                else if constexpr (std::is_same_v<T, WindowIconifyEvent>) {
                    // APP_LOG_DEBUG("Processing WindowIconifyEvent: iconified={}", arg.iconified);
                    if (m_window_iconify_callback) {
                        m_window_iconify_callback(arg.iconified);
                    }
                }
                else if constexpr (std::is_same_v<T, WindowCloseEvent>) {
                    // APP_LOG_DEBUG("WindowCloseEvent received");
                }
                else if constexpr (std::is_same_v<T, std::string>) {
                    // APP_LOG_DEBUG("Custom event: {}", arg);
                }
            },
            event);
    }
    m_events.clear();
}

void InputHandler::ApplyStateCallbacks(const std::string &state)
{
    if (auto it = m_state_scroll_callbacks.find(state); it != m_state_scroll_callbacks.end())
        m_scroll_callback = it->second;
    if (auto it = m_state_char_callbacks.find(state); it != m_state_char_callbacks.end())
        m_char_callback = it->second;
    if (auto it = m_state_cursor_enter_callbacks.find(state); it != m_state_cursor_enter_callbacks.end())
        m_cursor_enter_callback = it->second;
    if (auto it = m_state_window_focus_callbacks.find(state); it != m_state_window_focus_callbacks.end())
        m_window_focus_callback = it->second;
    if (auto it = m_state_framebuffer_size_callbacks.find(state); it != m_state_framebuffer_size_callbacks.end())
        m_frame_buffer_size_callback = it->second;
    if (auto it = m_state_window_size_callbacks.find(state); it != m_state_window_size_callbacks.end())
        m_window_size_callback = it->second;
    if (auto it = m_state_window_iconify_callbacks.find(state); it != m_state_window_iconify_callbacks.end())
        m_window_iconify_callback = it->second;
    ENGINE_LOG_DEBUG("Applied state-specific callbacks for '{}'", state);
}

} // namespace gouda