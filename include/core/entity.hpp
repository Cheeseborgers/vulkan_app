#pragma once
/**
 * @file entity.hpp
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
#include "math/math.hpp"
#include "renderers/vulkan/vk_renderer.hpp"

enum class EntityType : u8 {
    Quad = 0,  // Static quad
    Player,    // Player-controlled entity
    Enemy,     // AI-controlled entity
    Trigger,   // Interactive trigger
    Background // Static background
};

struct Entity {
    Entity(const gouda::vk::InstanceData &instance_data, EntityType type_ = EntityType::Quad)
        : type{type_}, render_data{instance_data}
    {
    }

    EntityType type;                     ///< Type of entity
    gouda::vk::InstanceData render_data; ///< Rendering-specific data
};
