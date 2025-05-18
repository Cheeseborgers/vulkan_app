#pragma once

/**
 * @file renderers/common.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-29
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
#include "core/types.hpp"
#include "math/math.hpp"

#include <vulkan/vulkan.h>

namespace gouda {

enum class PrimitiveTopology : u8 {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    TriangleFan,
    LineListWithAdjacency,
    LineStripWithAdjacency,
    TriangleListWithAdjacency,
    TriangleStripWithAdjacency,
    PatchList,
    None
};

constexpr VkPrimitiveTopology primitive_topology_to_vulkan(const PrimitiveTopology topology)
{
    switch (topology) {
        case PrimitiveTopology::PointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case PrimitiveTopology::LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveTopology::LineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case PrimitiveTopology::TriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::TriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PrimitiveTopology::TriangleFan:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        default:
            ENGINE_LOG_ERROR("Invalid primitive topology");
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    }
}

inline Vec4 get_normalized_sprite_rect(const f32 x_px, const f32 y_px, const f32 width_px, const f32 height_px,
                                      const f32 atlas_width, const f32 atlas_height)
{
    if (x_px < 0 || y_px < 0 || width_px <= 0 || height_px <= 0 || atlas_width <= 0 || atlas_height <= 0) {
        ENGINE_LOG_ERROR("Invalid sprite rect parameters");
        return Vec4{0.0f};
    }

    const f32 u_min = x_px / atlas_width;
    const f32 u_max = (x_px + width_px) / atlas_width;
    const f32 v_min = (atlas_height - (y_px + height_px)) / atlas_height;
    const f32 v_max = (atlas_height - y_px) / atlas_height;

    return Vec4{u_min, v_min, u_max, v_max};
}

} // namespace gouda