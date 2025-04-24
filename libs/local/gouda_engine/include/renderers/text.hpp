#pragma once
/**
 * @file text.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-08
 * @brief Engine msdf text module implementation
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
#include <string_view>
#include <unordered_map>

#include "core/types.hpp"
#include "math/math.hpp"

namespace gouda {

struct Glyph {
    Glyph();

    f32 advance;
    Rect<f32> plane_bounds;
    Rect<f32> atlas_bounds;
};

struct TextData {
    TextData();
    TextData(Vec3 position_, Vec2 size_, f32 rotation_, u32 texture_index_, Vec4 colour_ = Vec4(1.0f),
             Rect<f32> sprite_rect_ = Rect<f32>(0.0f), u32 is_atlas_ = 0);

    Vec3 position;         ///< World position (Z clamped between -0.9 and 0.0)
    Vec2 size;             ///< Size of the instance
    u32 texture_index;     ///< Index into texture array
    Vec4 colour;           ///< Tint colour
    Rect<f32> sprite_rect; ///< Sprite rectangle (for atlases)
};

std::unordered_map<char, Glyph> load_msdf_glyphs(std::string_view json_path);

} // namespace gouda