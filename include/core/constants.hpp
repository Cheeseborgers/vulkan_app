#pragma once

/**
 * @file constants.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-15
 * @brief Application module
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

constexpr std::string_view quad_vertex_shader_path{"assets/shaders/compiled/quad_shader.vert.spv"};
constexpr std::string_view quad_frag_shader_path{
    "assets/shaders/compiled/quad_shader.frag.spv",
};
constexpr std::string_view text_vertex_shader_path{"assets/shaders/compiled/text_shader.vert.spv"};
constexpr std::string_view text_frag_shader_path{"assets/shaders/compiled/text_shader.frag.spv"};
constexpr std::string_view particle_vertex_shader_path{"assets/shaders/compiled/particle_shader.vert.spv"};
constexpr std::string_view particle_frag_shader_path{"assets/shaders/compiled/particle_shader.frag.spv"};
constexpr std::string_view particle_compute_shader_path{"assets/shaders/compiled/particle_shader.comp.spv"};