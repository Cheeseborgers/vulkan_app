#pragma once
/**
 * @file health_component.hpp
 * @author GoudaCheeseburgers
 * @date 2025-05-26
 * @brief Application health component module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "core/types.hpp"

enum class HealthState : u8 {
    Alive,
    Deadge
};

struct HealthComponent {
    f32 current_hp{100.0f};
    f32 max_hp{100.0f};
    HealthState state{HealthState::Alive};

    void Damage(f32 dmg);
    void Heal(f32 amount);
    [[nodiscard]] bool IsAlive() const;
};
