#pragma once
/**
 * @file glfw_backend.hpp
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

#include <GLFW/glfw3.h>

#include "backends/input_backend.hpp"
#include "core/types.hpp"

namespace gouda {
namespace glfw {

inline void wait_events(f64 timeout) { glfwWaitEventsTimeout(static_cast<f64>(timeout)); }

class GLFWBackend : public InputBackend {
public:
    explicit GLFWBackend(std::function<void(Event)> eventCallback);
    void RegisterCallbacks(GLFWwindow *window) override;
    void PollEvents() override;

private:
    static Key ConvertKey(int glfw_key);

private:
    std::function<void(Event)> p_callback;
};
}
}
