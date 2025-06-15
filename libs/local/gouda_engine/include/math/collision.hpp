#pragma once
/**
 * @file math/collision.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-04
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
#include "math/math.hpp"

namespace gouda::math {

struct AABB2D {
    Vec2 min;
    Vec2 max;

    AABB2D() = default;
    AABB2D(const Vec2 &min_, const Vec2 &max_) : min(min_), max(max_) {}

    [[nodiscard]] bool Intersects(const AABB2D &other) const
    {
        return !(max.x < other.min.x || min.x > other.max.x ||
         max.y < other.min.y || min.y > other.max.y);
    }
};

struct AABB3D {
    Vec3 min;
    Vec3 max;

    AABB3D() = default;
    AABB3D(const Vec3 &min_, const Vec3 &max_) : min(min_), max(max_) {}

    [[nodiscard]] bool Intersects(const AABB3D &other) const
    {
        return (min.x <= other.max.x && max.x >= other.min.x) && (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};

inline bool check_collision(const Vec3 &pos1, const Vec2 &size1, const Vec3 &pos2, const Vec2 &size2)
{
    const bool collision_x{pos1.x + size1.x >= pos2.x && pos2.x + size2.x >= pos1.x};
    const bool collision_y{pos1.y + size1.y >= pos2.y && pos2.y + size2.y >= pos1.y};
    return collision_x && collision_y;
}

} // namespace gouda::math