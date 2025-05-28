#pragma once
/**
 * @file renderers/render_data.hpp
 * @author GoudaCheeseburgers
 * @date 2025-05-07
 * @brief Engine module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "core/types.hpp"
#include "math/math.hpp"

namespace gouda {

struct Vertex {
    Vertex(const Vec3 &pos, const Vec2 &tex_coords) : position{pos}, uv{tex_coords} {}

    Vec3 position;
    Vec2 uv;
};

struct alignas(16) UniformData {
    UniformData() : WVP{Mat4::identity()} {}

    Mat4 WVP;
};

struct InstanceData {
    InstanceData();
    InstanceData(const Vec3 &position, const Vec2 &size, f32 rotation, u32 texture_index,
                 const Colour<f32> &colour = Colour(1.0f),
                 const UVRect<f32> &sprite_rect = UVRect{0.0f, 0.0f, 0.0f, 0.0f}, u32 is_atlas = 0);

    Vec3 position; // 12 bytes, VK_FORMAT_R32G32B32_SFLOAT
    f32 _pad0;     // 4 bytes padding for alignment

    Vec2 size;         // 8 bytes, VK_FORMAT_R32G32_SFLOAT
    f32 rotation;      // 4 bytes, VK_FORMAT_R32_SFLOAT
    u32 texture_index; // 4 bytes, VK_FORMAT_R32_UINT

    Colour<f32> colour;      // 16 bytes, VK_FORMAT_R32G32B32A32_SFLOAT
    UVRect<f32> sprite_rect; // 16 bytes, VK_FORMAT_R32G32B32A32_SFLOAT

    u32 is_atlas; // 4 bytes, VK_FORMAT_R32_UINT
    u32 _pad1[3]; // 12 bytes padding to align to 16-byte boundary
};

struct alignas(16) TextData {
    TextData();

    Vec3 position; // 12 bytes, VK_FORMAT_R32G32B32_SFLOAT
    f32 _pad0;     // 4 bytes padding

    Vec2 size;       // 8 bytes, VK_FORMAT_R32G32_SFLOAT
    f32 _pad1;       // 4 bytes padding
    u32 glyph_index; // 4 bytes, VK_FORMAT_R32_UINT

    Colour<f32> colour;     // 16 bytes, VK_FORMAT_R32G32B32A32_SFLOAT
    UVRect<f32> sdf_params; // 16 bytes, VK_FORMAT_R32G32B32A32_SFLOAT

    u32 texture_index; // 4 bytes, VK_FORMAT_R32_UINT
    Vec2 atlas_size;   // 8 bytes, VK_FORMAT_R32G32_SFLOAT
    f32 px_range;      // 4 bytes, VK_FORMAT_R32_SFLOAT

    u32 _pad2[3]; // 12 bytes padding to align to 16-byte boundary
};

struct SimulationParams {
    SimulationParams();
    SimulationParams(const Vec3 &gravity, f32 delta_time);

    Vec3 gravity;   // 12 bytes, VK_FORMAT_R32G32B32_SFLOAT
    f32 delta_time; // 4 bytes, VK_FORMAT_R32_SFLOAT
    // Total: 16 bytes
};

struct alignas(16) ParticleData {
    ParticleData(const Vec3 &position_, const Vec2 &size_, const f32 lifetime_, const Vec3 &velocity_,
                 const Vec4 &colour_, const u32 texture_index_,
                 const UVRect<f32> &sprite_rect_ = UVRect<f32>{0.0f, 0.0f, 0.0f, 0.0f}, const u32 is_atlas_ = 0)
        : position{position_},
          size{size_},
          lifetime{lifetime_},
          velocity{velocity_},
          colour{colour_},
          texture_index{texture_index_},
          sprite_rect{sprite_rect_},
          is_atlas{is_atlas_}
    {
    }

    Vec3 position; // offset 0
    f32 _pad0{};   // offset 12 → pad vec3 to vec4

    Vec2 size;      // offset 16
    f32 lifetime{}; // offset 24
    f32 _pad1{};    // offset 28 → pad to 32

    Vec3 velocity; // offset 32
    f32 _pad2{};   // offset 44 → pad to 48

    Vec4 colour; // offset 48

    u32 texture_index{}; // offset 64
    f32 _pad3[3]{};      // offset 68 → pad to 80

    UVRect<f32> sprite_rect; // offset 80

    u32 is_atlas{}; // offset 96
    f32 _pad4[3]{}; // offset 100 → pad to 112
};

} // namespace gouda