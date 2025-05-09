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
                 Vec4 sprite_rect_ = Vec4(0.0f), u32 is_atlas_ = 0);

    Vec3 position; // VK_FORMAT_R32G32B32_SFLOAT
    Vec2 size;     // VK_FORMAT_R32G32_SFLOAT
    f32 rotation;
    u32 texture_index; // VK_FORMAT_R32_UINT
    Vec4 colour;       // VK_FORMAT_R32G32B32A32_SFLOAT
    Vec4 sprite_rect;
    u32 is_atlas; // VK_FORMAT_R32_UINT
};

struct TextData {
    TextData();

    Vec3 position;     // VK_FORMAT_R32G32B32_SFLOAT
    Vec2 size;         // VK_FORMAT_R32G32_SFLOAT
    Vec4 colour;       // VK_FORMAT_R32G32B32A32_SFLOAT
    u32 glyph_index;   // VK_FORMAT_R32_UINT
    Vec4 sdf_params;   // VK_FORMAT_R32G32B32A32_SFLOAT
    u32 texture_index; // VK_FORMAT_R32_UINT
};

struct SimulationParams {
    f32 deltaTime;
    f32 gravity[3];
};

struct ParticleData {
    Vec3 position;     // 12 bytes
    f32 _pad0;         // 4 bytes padding for vec3
    Vec2 size;         // 8 bytes
    Vec4 colour;       // 16 bytes
    u32 texture_index; // 4 bytes
    f32 lifetime;      // 4 bytes
    Vec3 velocity;     // 12 bytes
    f32 _pad1;         // 4 bytes padding for vec3
    // Total: 64 bytes
};

} // namespace gouda