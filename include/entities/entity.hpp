#pragma once
/**
 * @file entities/entity.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-06
 * @brief Application module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <optional>

#include "components/animation_component.hpp"
#include  "components/health_component.hpp"

enum class EntityType : u8 {
    Quad = 0,  // Static quad
    Player,    // Player-controlled entity
    Enemy,     // AI-controlled entity
    Trigger,   // Interactive trigger
    Background // Static background
};

struct Entity {
    explicit Entity(const gouda::InstanceData &instance_data, const EntityType type_ = EntityType::Quad)
        : type{type_}, render_data{instance_data}
    {
    }

    Rect<f32> GetBounds() const; // TODO: Remove this and compute bounds else where. ie: collider component.

    EntityType type;                                       ///< Type of entity
    gouda::InstanceData render_data;                       ///< Rendering-specific data
    std::optional<AnimationComponent> animation_component; ///< Optional animation component
    std::optional<HealthComponent> health_component;       ///< Optional health component
};
