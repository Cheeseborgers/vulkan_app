#pragma once

/**
 * @file mouse_code.hpp
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
#include <cstdint>

namespace gouda {

// Enum class for mouse buttons
enum class MouseButton : uint8_t {
    Left = 0,   // GLFW_MOUSE_BUTTON_LEFT
    Right = 1,  // GLFW_MOUSE_BUTTON_RIGHT
    Middle = 2, // GLFW_MOUSE_BUTTON_MIDDLE
    Button3 = 3,
    Button4 = 4,
    Button5 = 5,
    Button6 = 6,
    Button7 = 7,
    None,
};

} // namespace gouda