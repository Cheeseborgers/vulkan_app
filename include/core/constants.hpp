#pragma once
/**
 * @file constants.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-15
 * @brief Application module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "core/types.hpp"

namespace filepath {
constexpr StringView application_icon{"assets/textures/gouda_icon.png"};

// Textures
constexpr StringView texture_atlas{"assets/textures/sprite_sheet.png"};
constexpr StringView texture_atlas_metadata{"assets/textures/sprite_sheet.json"};

// Shaders
constexpr StringView quad_vertex_shader{"assets/shaders/compiled/quad_shader.vert.spv"};
constexpr StringView quad_frag_shader{"assets/shaders/compiled/quad_shader.frag.spv"};
constexpr StringView text_vertex_shader{"assets/shaders/compiled/text_shader.vert.spv"};
constexpr StringView text_frag_shader{"assets/shaders/compiled/text_shader.frag.spv"};
constexpr StringView particle_vertex_shader{"assets/shaders/compiled/particle_shader.vert.spv"};
constexpr StringView particle_frag_shader{"assets/shaders/compiled/particle_shader.frag.spv"};
constexpr StringView particle_compute_shader{"assets/shaders/compiled/particle_shader.comp.spv"};

// Fonts
constexpr StringView primary_font_atlas{"assets/fonts/firacode_atlas.png"};
constexpr StringView primary_font_metadata{"assets/fonts/firacode_atlas.json"};
constexpr StringView secondary_font_atlas{"assets/fonts/roboto_atlas.png"};
constexpr StringView secondary_font_metadata{"assets/fonts/roboto_atlas.json"};

// Sounds
} // namespace filepath

namespace colours {
// Colours
constexpr  gouda::Colour<f32> debug_panel_colour{0.1f, 0.8f, 0.6f, 1.0f};
constexpr  gouda::Colour<f32> editor_panel_colour{0.1f, 0.1f, 0.1f, 1.0f};
constexpr  gouda::Colour<f32> editor_panel_primary_font_colour{0.4f, 0.3f, 0.7f, 1.0f};
constexpr  gouda::Colour<f32> editor_panel_secondary_font_colour{0.4f, 0.3f, 0.7f, 1.0f};
constexpr  gouda::Colour<f32> editor_entity_selection_colour{0.3f, 0.9f, 0.3f, 1.0f};
} // namespace colour

// TODO: Move all constants here
namespace app_constants {
constexpr u32 max_quads{1000};
constexpr u32 max_particles{1000};
constexpr u32 max_glyphs{1000};

} // namespace app_constants
