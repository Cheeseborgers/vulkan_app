#pragma once

/**
 * @file keycodes.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-11
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
#include "core/types.hpp"

namespace gouda {

enum class Key : u16 {
    None,
    Unknown, // Explicitly handle unknown keys
    Space,
    Apostrophe, /* ' */
    Comma,      /* , */
    Minus,      /* - */
    Period,     /* . */
    Slash,      /* / */

    D0, /* 0 */
    D1, /* 1 */
    D2, /* 2 */
    D3, /* 3 */
    D4, /* 4 */
    D5, /* 5 */
    D6, /* 6 */
    D7, /* 7 */
    D8, /* 8 */
    D9, /* 9 */

    Semicolon, /* ; */
    Equal,     /* = */

    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    LeftBracket,  /* [ */
    Backslash,    /* \ */
    RightBracket, /* ] */
    GraveAccent,  /* ` */

    World1, /* non-US #1 */
    World2, /* non-US #2 */

    /* Function keys */
    Escape,
    Enter,
    Tab,
    Backspace,
    Insert,
    Delete,
    Right,
    Left,
    Down,
    Up,
    PageUp,
    PageDown,
    Home,
    End,
    CapsLock,
    ScrollLock,
    NumLock, // NumLock (standard)
    PrintScreen,
    Pause,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,
    F25,

    /* Keypad */
    KP0,
    KP1,
    KP2,
    KP3,
    KP4,
    KP5,
    KP6,
    KP7,
    KP8,
    KP9,
    KPDecimal,
    KPDivide,
    KPMultiply,
    KPSubtract,
    KPAdd,
    KPEnter,
    KPEqual,
    KPNumLock, // New: Some keypads treat Num Lock separately

    LeftShift,
    LeftControl,
    LeftAlt,
    LeftSuper,
    RightShift,
    RightControl,
    RightAlt,
    RightSuper,
    Menu
};

namespace internal {

constexpr std::array<const char *, 123> keystrings = {"None",
                                                      "Unknown",
                                                      "Space",
                                                      "Apostrophe",
                                                      "Comma",
                                                      "Minus",
                                                      "Period",
                                                      "Slash",
                                                      "D0",
                                                      "D1",
                                                      "D2",
                                                      "D3",
                                                      "D4",
                                                      "D5",
                                                      "D6",
                                                      "D7",
                                                      "D8",
                                                      "D9",
                                                      "Semicolon",
                                                      "Equal",
                                                      "A",
                                                      "B",
                                                      "C",
                                                      "D",
                                                      "E",
                                                      "F",
                                                      "G",
                                                      "H",
                                                      "I",
                                                      "J",
                                                      "K",
                                                      "L",
                                                      "M",
                                                      "N",
                                                      "O",
                                                      "P",
                                                      "Q",
                                                      "R",
                                                      "S",
                                                      "T",
                                                      "U",
                                                      "V",
                                                      "W",
                                                      "X",
                                                      "Y",
                                                      "Z",
                                                      "LeftBracket",
                                                      "Backslash",
                                                      "RightBracket",
                                                      "GraveAccent",
                                                      "World1",
                                                      "World2",
                                                      "Escape",
                                                      "Enter",
                                                      "Tab",
                                                      "Backspace",
                                                      "Insert",
                                                      "Delete",
                                                      "Right",
                                                      "Left",
                                                      "Down",
                                                      "Up",
                                                      "PageUp",
                                                      "PageDown",
                                                      "Home",
                                                      "End",
                                                      "CapsLock",
                                                      "ScrollLock",
                                                      "NumLock",
                                                      "PrintScreen",
                                                      "Pause",
                                                      "F1",
                                                      "F2",
                                                      "F3",
                                                      "F4",
                                                      "F5",
                                                      "F6",
                                                      "F7",
                                                      "F8",
                                                      "F9",
                                                      "F10",
                                                      "F11",
                                                      "F12",
                                                      "F13",
                                                      "F14",
                                                      "F15",
                                                      "F16",
                                                      "F17",
                                                      "F18",
                                                      "F19",
                                                      "F20",
                                                      "F21",
                                                      "F22",
                                                      "F23",
                                                      "F24",
                                                      "F25",
                                                      "KP0",
                                                      "KP1",
                                                      "KP2",
                                                      "KP3",
                                                      "KP4",
                                                      "KP5",
                                                      "KP6",
                                                      "KP7",
                                                      "KP8",
                                                      "KP9",
                                                      "KPDecimal",
                                                      "KPDivide",
                                                      "KPMultiply",
                                                      "KPSubtract",
                                                      "KPAdd",
                                                      "KPEnter",
                                                      "KPEqual",
                                                      "KPNumLock",
                                                      "LeftShift",
                                                      "LeftControl",
                                                      "LeftAlt",
                                                      "LeftSuper",
                                                      "RightShift",
                                                      "RightControl",
                                                      "RightAlt",
                                                      "RightSuper",
                                                      "Menu"};

} // namespace internal

inline std::string KeyToString(Key key)
{
    size_t index = static_cast<size_t>(key);
    if (index >= internal::keystrings.size()) {
        return "Unknown"; // Out of bounds, return "Unknown"
    }
    return internal::keystrings[index];
}

} // namespace gouda