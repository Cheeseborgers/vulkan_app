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
#include "containers/small_vector.hpp"

#include <unordered_map>

#include "core/types.hpp"
#include "math/math.hpp"

namespace gouda {

enum class TextAlign : u8 { Left, Center, Right };

struct MSDFGlyph {
    MSDFGlyph();

    f32 advance;
    Rect<f32> plane_bounds;
    Rect<f32> atlas_bounds;
};

using MSDFGlyphMap = std::unordered_map<char, MSDFGlyph>;

struct alignas(16) MSDFKerningPair {
    MSDFKerningPair();

    u32 unicode1;
    u32 unicode2;
    f32 advance;
    f32 _padding1;
};

struct alignas(16) MSDFAtlasParams {
    MSDFAtlasParams();

    [[nodiscard]] std::string ToString() const;

    f32 distance_range;
    f32 distance_range_middle;
    f32 font_size;
    f32 _padding0; // pad to next 16-byte boundary

    Vec2 atlas_size; // 8 bytes
    f32 em_size;
    f32 line_height;

    f32 ascender;
    f32 descender;
    f32 underline_y;
    f32 underline_thickness;

    alignas(16) Vector<MSDFKerningPair> kerning;

    alignas(4) bool y_origin_is_bottom;
    u8 _padding1[3]; // pad bool to 4 bytes
    u32 _padding2;   // pad to next 16-byte boundary
};

std::unordered_map<char, MSDFGlyph> load_msdf_glyphs(StringView json_path);

MSDFAtlasParams load_msdf_atlas_params(StringView json_path);

} // namespace gouda