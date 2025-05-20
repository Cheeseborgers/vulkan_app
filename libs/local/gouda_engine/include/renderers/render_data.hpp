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

struct UniformData {
    UniformData() : WVP{Mat4::identity()} {}

    Mat4 WVP;
};

struct InstanceData {
    InstanceData();
    InstanceData(Vec3 position_, Vec2 size_, f32 rotation_, u32 texture_index_, Vec4 colour_ = Vec4(1.0f),
                 UVRect<f32> sprite_rect_ = UVRect{0.0f, 0.0f, 0.0f, 0.0f}, u32 is_atlas_ = 0);

    Vec3 position; // 12 bytes, VK_FORMAT_R32G32B32_SFLOAT
    f32 _pad0;     // 4 bytes padding for alignment

    Vec2 size;         // 8 bytes, VK_FORMAT_R32G32_SFLOAT
    f32 rotation;      // 4 bytes, VK_FORMAT_R32_SFLOAT
    u32 texture_index; // 4 bytes, VK_FORMAT_R32_UINT

    Vec4 colour;             // 16 bytes, VK_FORMAT_R32G32B32A32_SFLOAT
    UVRect<f32> sprite_rect; // 16 bytes, VK_FORMAT_R32G32B32A32_SFLOAT

    u32 is_atlas; // 4 bytes, VK_FORMAT_R32_UINT
    u32 _pad1[3]; // 12 bytes padding to align to 16-byte boundary
    // Total: 96 bytes
};

struct TextData {
    TextData();

    Vec3 position; // 12 bytes, VK_FORMAT_R32G32B32_SFLOAT
    f32 _pad0;     // 4 bytes padding

    Vec2 size;       // 8 bytes, VK_FORMAT_R32G32_SFLOAT
    f32 _pad1;       // 4 bytes padding
    u32 glyph_index; // 4 bytes, VK_FORMAT_R32_UINT

    Vec4 colour;     // 16 bytes, VK_FORMAT_R32G32B32A32_SFLOAT
    Vec4 sdf_params; // 16 bytes, VK_FORMAT_R32G32B32A32_SFLOAT

    u32 texture_index; // 4 bytes, VK_FORMAT_R32_UINT
    u32 _pad2[3];      // 12 bytes padding to align to 16-byte boundary
    // Total: 96 bytes
};

struct SimulationParams {
    SimulationParams();
    SimulationParams(const Vec3 &gravity, f32 delta_time);

    Vec3 gravity;   // 12 bytes, VK_FORMAT_R32G32B32_SFLOAT
    f32 delta_time; // 4 bytes, VK_FORMAT_R32_SFLOAT
    // Total: 16 bytes
};

struct ParticleData {
    ParticleData();
    ParticleData(const Vec3 &position_, const Vec2 &size_, const Vec4 &colour_, u32 texture_index_, f32 lifetime_,
                 const Vec3 &velocity_);

    Vec3 position; // 12 bytes, VK_FORMAT_R32G32B32_SFLOAT
    f32 _pad0;     // 4 bytes padding

    Vec2 size;         // 8 bytes, VK_FORMAT_R32G32_SFLOAT
    Vec4 colour;       // 16 bytes, VK_FORMAT_R32G32B32A32_SFLOAT
    u32 texture_index; // 4 bytes, VK_FORMAT_R32_UINT
    f32 lifetime;      // 4 bytes, VK_FORMAT_R32_SFLOAT
    Vec3 velocity;     // 12 bytes, VK_FORMAT_R32G32B32_SFLOAT
    f32 _pad1;         // 4 bytes padding to align to 16-byte boundary
    // Total: 64 bytes
};

} // namespace gouda