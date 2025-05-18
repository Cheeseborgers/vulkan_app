/**
 * @file render_data.cpp
 * @author GoudaCheeseburgers
 * @date 2025-05-07
 * @brief Engine shared render data module implementation
 */
#include "renderers/render_data.hpp"

namespace gouda {



InstanceData::InstanceData()
    : position{0.0f, 0.0f, 0.0f}, size{1.0f, 1.0f}, rotation{0.0f}, texture_index{0}, colour{1.0f}, is_atlas(0)
{
}

InstanceData::InstanceData(Vec3 position_, Vec2 size_, f32 rotation_, u32 texture_index_, Vec4 colour_,
                           Vec4 sprite_rect_, u32 is_atlas_)
    : position{position_},
      size{size_},
      rotation{rotation_},
      texture_index{texture_index_},
      colour{colour_},
      sprite_rect{sprite_rect_},
      is_atlas{is_atlas_}
{
    position.z = math::clamp(position.z, -0.9f, 0.0f);
}

TextData::TextData()
    : position{0.0f, 0.0f, 0.0f}, size{1.0f, 1.0f}, colour{0.0f}, glyph_index{0}, sdf_params{0.0f}, texture_index{0}
{
}

} // namespace gouda
