/**
 * @file render_data.cpp
 * @author GoudaCheeseburgers
 * @date 2025-05-07
 * @brief Engine shared render data module implementation
 */
#include "renderers/render_data.hpp"

namespace gouda {

InstanceData::InstanceData()
    : position{0.0f, 0.0f, 0.0f},
      _pad0{0.0f},
      size{1.0f, 1.0f},
      rotation{0.0f},
      texture_index{0},
      colour{1.0f, 0.0f, 0.0f, 1.0f}, // Default to red
      sprite_rect{0.0f, 0.0f, 0.0f, 0.0f},
      is_atlas{0},
      _pad1{}
{
}

InstanceData::InstanceData(const Vec3 &position_, const Vec2 &size_, const f32 rotation_, const u32 texture_index_,
                           const Colour<f32> &colour_, const UVRect<f32> &sprite_rect_, const u32 is_atlas_)
    : position{position_},
      _pad0{0.0f},
      size{size_},
      rotation{rotation_},
      texture_index{texture_index_},
      colour{colour_},
      sprite_rect{sprite_rect_},
      is_atlas{is_atlas_},
      _pad1{}
{
    // Ensure within orthographic view
    position.z = math::clamp(position.z, constants::z_min, constants::z_max);
}

TextData::TextData()
    : position{0.0f, 0.0f, 0.0f},
      _pad0{0.0f},
      size{1.0f, 1.0f},
      _pad1{0.0f},
      glyph_index{0},
      colour{1.0f, 0.0f, 0.0f, 1.0f}, // Default to red
      sdf_params{0.0f, 0.0f, 0.0f, 0.0f},
      texture_index{0},
      _pad2{}
{
}

SimulationParams::SimulationParams() : gravity{0.0f}, delta_time{0.0f} {}
SimulationParams::SimulationParams(const Vec3 &gravity_, const f32 delta_time_)  : gravity{gravity_}, delta_time{delta_time_}
{}





} // namespace gouda
