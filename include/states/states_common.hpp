#pragma once
/**
 * @file states/states_common.hpp
 * @author GoudaCheeseburgers
 * @date 2025-15-06
 * @brief Application common types and functions used in states
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "containers/small_vector.hpp"
#include "core/types.hpp"
#include "math/collision.hpp"
#include "math/math.hpp"

struct GridPos {
    s32 x;
    s32 y;

    bool operator==(const GridPos &other) const { return x == other.x && y == other.y; }
    struct Hash {
        size_t operator()(const GridPos &pos) const { return std::hash<int>()(pos.x) ^ std::hash<int>()(pos.y) << 1; }
    };
};

using SpatialGrid = std::unordered_map<GridPos, gouda::Vector<size_t>, GridPos::Hash>;

struct GridRange {
    s32 min_x;
    s32 max_x;
    s32 min_y;
    s32 max_y;
};

inline GridRange GetGridRange(const Rect<f32> &bounds, const f32 cell_size)
{
    return {(gouda::math::floor(bounds.left / cell_size)), (gouda::math::floor(bounds.right / cell_size)),
            (gouda::math::floor(bounds.bottom / cell_size)), (gouda::math::floor(bounds.top / cell_size))};
}

inline bool IsInFrustum(const gouda::Vec3 &position, const gouda::Vec2 &size,
                        const gouda::OrthographicCamera::FrustumData &frustum)
{
    const gouda::math::AABB2D frustum_bounds{{frustum.left + frustum.position.x, frustum.top + frustum.position.y},
                                             {frustum.right + frustum.position.x, frustum.bottom + frustum.position.y}};
    const gouda::math::AABB2D object_bounds{{position.x, position.y}, {position.x + size.x, position.y + size.y}};

    return object_bounds.Intersects(frustum_bounds);
}
