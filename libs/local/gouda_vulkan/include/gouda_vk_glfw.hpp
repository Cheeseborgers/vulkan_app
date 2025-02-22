/*

        Copyright 2024 GoudaCheeseburgers

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <functional>
#include <string_view>

#include <GLFW/glfw3.h>

#include "gouda_types.hpp"

// TODO: Implement logging fully

namespace GoudaVK {
namespace GLFW {

enum class Platform { X11, Wayland, Windows, Headless, SystemDefault };

using KeyCallback = std::function<void(GLFWwindow *, int, int, int, int)>;
using MouseMoveCallback = std::function<void(GLFWwindow *, double, double)>;
using MouseButtonCallback = std::function<void(GLFWwindow *, int, int, int)>;
using MouseScrollCallback = std::function<void(GLFWwindow *, double, double)>;
using WindowIconifyCallback = std::function<void(GLFWwindow *, int)>;
using FramebufferResizedCallback = std::function<void(GLFWwindow *, FrameBufferSize)>;

struct Callbacks {
    KeyCallback key_callback;
    MouseMoveCallback mouse_move_callback;
    MouseButtonCallback mouse_button_callback;
    MouseScrollCallback mouse_scroll_callback;
    WindowIconifyCallback window_iconify_callback;
    FramebufferResizedCallback framebuffer_resized_callback;
};

std::optional<GLFWwindow *> vulkan_init(WindowSize window_size, std::string_view application_title,
                                        bool window_resizable = true, Platform platform = Platform::X11);

void set_callbacks(GLFWwindow *window_ptr, GLFW::Callbacks *callbacks_ptr);

// void set_window_icon(std::string_view file_path);

} // end GLFW namespace
} // end GoudaVK namespace
