#pragma once
/**
 * @file entities/player.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-06
 * @brief Application player entity module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "entities/entity.hpp"

struct Player : public Entity {
public:
    Player(gouda::InstanceData instance_data, gouda::math::Vec2 velocity_, f32 speed_);
    ~Player();

    gouda::math::Vec2 velocity; ///< Movement velocity
    f32 speed;                  ///< Movement speed
};