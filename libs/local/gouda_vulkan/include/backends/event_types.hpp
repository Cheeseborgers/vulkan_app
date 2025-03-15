#pragma once
/**
 * @file backends/event_types.hpp
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
#include <string>
#include <variant>

#include "backends/keycodes.hpp"
#include "backends/mouse_codes.hpp"

namespace gouda {

// Enum for input actions
enum class ActionState { Pressed, Released, Repeated };

// Typing event class
struct CharEvent {
    unsigned int codepoint; // Unicode codepoint
};

// When the cursor enters or leaves the window
struct CursorEnterEvent {
    bool entered;
};

// Event data structs
struct KeyEvent {
    Key key;
    ActionState state;
    int mods; // Modifier keys (e.g., Shift, Ctrl)
};

struct MouseButtonEvent {
    MouseButton button;
    ActionState state;
    int mods; // Modifier keys
};

struct MouseMoveEvent {
    double x;
    double y;
};

struct MouseScrollEvent {
    double xOffset;
    double yOffset;
};

struct WindowCloseEvent {};

struct WindowFocusEvent {
    bool focused;
};

struct WindowFramebufferSizeEvent {
    int width;
    int height;
};

struct WindowSizeEvent {
    int width, height;
};

struct WindowIconifyEvent {
    bool iconified;
};

using Event = std::variant<KeyEvent, MouseButtonEvent, MouseMoveEvent, MouseScrollEvent, WindowCloseEvent, CharEvent,
                           CursorEnterEvent, WindowFocusEvent, WindowFramebufferSizeEvent, WindowSizeEvent,
                           WindowIconifyEvent, std::string>;

} // namespace gouda