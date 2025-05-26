/**
* @file health_component.cpp
 * @author GoudaCheeseburgers
 * @date 2025-05-07
 * @brief Application health component module implementation
 */
#include "components/health_component.hpp"

#include "math/math.hpp"

void HealthComponent::Damage(const f32 dmg)
{
    current_hp = gouda::math::max(current_hp - dmg, 0.0f);
    if (current_hp == 0) {
        state = HealthState::Deadge;
    }
}
void HealthComponent::Heal([[maybe_unused]] const f32 amount)
{
    // TODO: add implentation
}

bool HealthComponent::IsAlive() const { return state == HealthState::Alive; }