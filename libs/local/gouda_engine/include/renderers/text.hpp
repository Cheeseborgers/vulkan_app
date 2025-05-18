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
#include <unordered_map>

#include "core/types.hpp"

namespace gouda {

struct Glyph {
    Glyph();

    f32 advance;
    Rect<f32> plane_bounds;
    Rect<f32> atlas_bounds;
};

std::unordered_map<char, Glyph> load_msdf_glyphs(StringView json_path);

} // namespace gouda