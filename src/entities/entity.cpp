/**
 * @file entity.cpp
 * @author GoudaCheeseburgers
 * @date 2025-04-06
 * @brief Application module implementation
 */
#include "entities/entity.hpp"

Rect<f32> Entity::GetBounds() const
{
    return Rect<f32>{
        render_data.position.x,                      // left
        render_data.position.x + render_data.size.x, // right
        render_data.position.y,                      // bottom
        render_data.position.y + render_data.size.y  // top
    };
}
