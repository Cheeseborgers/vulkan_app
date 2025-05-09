/**
 * @file entities/player.cpp
 * @author GoudaCheeseburgers
 * @date 2025-04-06
 * @brief Application player module implementation
 */

#include "entities/player.hpp"

Player::Player(gouda::InstanceData instance_data, gouda::math::Vec2 velocity_, f32 speed_)
    : Entity{instance_data, EntityType::Player}, velocity{velocity_}, speed{speed_}
{
}

Player::~Player() {}
