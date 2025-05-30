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
      apply_camera_effects{1}, // Default to true
      _pad1{}
{
}

InstanceData::InstanceData(const Vec3 &position_, const Vec2 &size_, const f32 rotation_, const u32 texture_index_,
                           const Colour<f32> &colour_, const UVRect<f32> &sprite_rect_, const u32 is_atlas_,
                           const u32 apply_camera_effects_)
    : position{position_},
      _pad0{0.0f},
      size{size_},
      rotation{rotation_},
      texture_index{texture_index_},
      colour{colour_},
      sprite_rect{sprite_rect_},
      is_atlas{is_atlas_},
      apply_camera_effects{apply_camera_effects_},
      _pad1{}
{

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
      atlas_size{0.0f, 0.0f},
      px_range{0.0f},
      apply_camera_effects{1} // Default to false
{
}

SimulationParams::SimulationParams() : gravity{0.0f}, delta_time{0.0f} {}
SimulationParams::SimulationParams(const Vec3 &gravity_, const f32 delta_time_)
    : gravity{gravity_}, delta_time{delta_time_}
{
}

ParticleData::ParticleData(const Vec3 &position_, const Vec2 &size_, const f32 lifetime_, const Vec3 &velocity_,
                           const Vec4 &colour_, const u32 texture_index_, const UVRect<f32> &sprite_rect_,
                           const u32 is_atlas_, const u32 apply_camera_effects_)
    : position{position_},
      size{size_},
      lifetime{lifetime_},
      velocity{velocity_},
      colour{colour_},
      texture_index{texture_index_},
      sprite_rect{sprite_rect_},
      is_atlas{is_atlas_},
      apply_camera_effects{apply_camera_effects_}
{
}

} // namespace gouda
