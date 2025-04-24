#pragma once
/**
 * @file player.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-06
 * @brief Application module
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
#include "core/entity.hpp"

struct Player : public Entity {
public:
    Player(gouda::vk::InstanceData instance_data, gouda::math::Vec2 velocity_, f32 speed_);
    ~Player();

    gouda::math::Vec2 velocity; ///< Movement velocity
    f32 speed;                  ///< Movement speed
};