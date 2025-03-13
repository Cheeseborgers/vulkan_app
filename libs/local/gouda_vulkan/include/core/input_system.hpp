#pragma once

/**
 * @file input_system.hpp
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
#include <functional>
#include <string>
#include <unordered_map>

#include "input_event.hpp"

namespace gouda {
class InputSystem {
public:
    using Callback = std::function<void(const InputEvent &)>;

    void BindAction(const std::string &state, const InputEvent &event, Callback callback);
    void UnbindState(const std::string &state);
    void ProcessEvent(const InputEvent &event, const std::string &currentState);

    void SaveToJson(const std::string &filename) const;
    void LoadFromJson(const std::string &filename);

private:
    std::unordered_map<std::string, std::unordered_map<InputEvent, Callback>> stateBindings_;
};

} // namespace gouda
