#pragma once
/**
 * @file backends/input_handler.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-14
 * @brief Engine module
 *
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <functional>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

#include "backends/input_backend.hpp"

struct GLFWwindow;

namespace gouda {

class InputHandler {
public:
    explicit InputHandler(std::unique_ptr<InputBackend> backend, GLFWwindow *window);

    using ActionCallback = std::function<void()>;
    using InputType = std::variant<Key, MouseButton>;
    using ScrollCallback = std::function<void(double, double)>;
    using CharCallback = std::function<void(unsigned int)>;
    using CursorEnterCallback = std::function<void(bool)>;
    using WindowFocusCallback = std::function<void(bool)>;
    using FramebufferSizeCallback = std::function<void(int, int)>;
    using WindowSizeCallback = std::function<void(int, int)>;
    using WindowIconifyCallback = std::function<void(bool)>;

    struct ActionBinding {
        InputType input;
        ActionState trigger_state;
        ActionCallback callback;
    };

    void LoadStateBindings(const std::string &state, const std::vector<ActionBinding> &bindings);
    void UnloadStateBindings(const std::string &state);
    void SetActiveState(const std::string &state);
    void PushCustomEvent(const std::string &event);
    void QueueEvent(Event event);

    // Standard callback setters
    void SetScrollCallback(ScrollCallback callback);
    void SetCharCallback(CharCallback callback);
    void SetCursorEnterCallback(CursorEnterCallback callback);
    void SetWindowFocusCallback(WindowFocusCallback callback);
    void SetFramebufferSizeCallback(FramebufferSizeCallback callback);
    void SetWindowSizeCallback(WindowSizeCallback callback);
    void SetWindowIconifyCallback(WindowIconifyCallback callback);

    // State-specific callback setters
    void SetStateScrollCallback(const std::string &state, ScrollCallback callback);
    void SetStateCharCallback(const std::string &state, CharCallback callback);
    void SetStateCursorEnterCallback(const std::string &state, CursorEnterCallback callback);
    void SetStateWindowFocusCallback(const std::string &state, WindowFocusCallback callback);
    void SetStateFramebufferSizeCallback(const std::string &state, FramebufferSizeCallback callback);
    void SetStateWindowSizeCallback(const std::string &state, WindowSizeCallback callback);
    void SetStateWindowIconifyCallback(const std::string &state, WindowIconifyCallback callback);

    // Clear callback methods
    void ClearScrollCallback();
    void ClearCharCallback();
    void ClearCursorEnterCallback();
    void ClearWindowFocusCallback();
    void ClearFramebufferSizeCallback();
    void ClearWindowSizeCallback();
    void ClearWindowIconifyCallback();

    bool IsKeyPressed(Key key) const;
    bool IsMouseButtonPressed(MouseButton button) const;
    std::pair<double, double> GetMousePosition() const;

    void Update();

private:
    void ProcessEvents();
    void ApplyStateCallbacks(const std::string &state);

    std::unique_ptr<InputBackend> p_backend;
    std::unordered_map<std::string, std::vector<ActionBinding>> m_bindings;
    std::string m_active_state;
    std::vector<Event> m_events;
    GLFWwindow *p_window;

    ScrollCallback m_scroll_callback;
    CharCallback m_char_callback;
    CursorEnterCallback m_cursor_enter_callback;
    WindowFocusCallback m_window_focus_callback;
    FramebufferSizeCallback m_frame_buffer_size_callback;
    WindowSizeCallback m_window_size_callback;
    WindowIconifyCallback m_window_iconify_callback;

    // State-specific callback maps
    std::unordered_map<std::string, ScrollCallback> m_state_scroll_callbacks;
    std::unordered_map<std::string, CharCallback> m_state_char_callbacks;
    std::unordered_map<std::string, CursorEnterCallback> m_state_cursor_enter_callbacks;
    std::unordered_map<std::string, WindowFocusCallback> m_state_window_focus_callbacks;
    std::unordered_map<std::string, FramebufferSizeCallback> m_state_framebuffer_size_callbacks;
    std::unordered_map<std::string, WindowSizeCallback> m_state_window_size_callbacks;
    std::unordered_map<std::string, WindowIconifyCallback> m_state_window_iconify_callbacks;

    // Input state tracking
    std::unordered_map<Key, bool> m_key_states;           // True if key is pressed
    std::unordered_map<MouseButton, bool> m_mouse_states; // True if button is pressed
    double m_mouse_x;                                     // Current mouse X position
    double m_mouse_y;
};

} // namespace gouda