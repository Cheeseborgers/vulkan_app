/**
 * @file entities/player.cpp
 * @author GoudaCheeseburgers
 * @date 2025-04-06
 * @brief Application player module implementation
 */

#include "entities/player.hpp"

Player::Player(const gouda::InstanceData &instance_data, const gouda::math::Vec2 velocity_, const f32 speed_)
    : Entity{instance_data, EntityType::Player}, velocity{velocity_}, speed{speed_}
{
}

Player::~Player() = default;
