#pragma once
/**
 * @file camera/camera.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-12
 *
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/types.hpp"

#include "math/math.hpp"
#include "math/random.hpp"

namespace gouda {

enum class CameraMovement : u32 {
    NONE = 0,
    MOVE_LEFT = 1 << 0,
    MOVE_RIGHT = 1 << 1,
    MOVE_UP = 1 << 2,
    MOVE_DOWN = 1 << 3,
    ZOOM_IN = 1 << 4,
    ZOOM_OUT = 1 << 5,
    ROTATE_X = 1 << 6, // For 3D
    ROTATE_Y = 1 << 7, // For 3D
};

inline CameraMovement operator|(CameraMovement lhs, CameraMovement rhs)
{
    return static_cast<CameraMovement>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
}

inline CameraMovement &operator|=(CameraMovement &lhs, CameraMovement rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline bool operator&(CameraMovement lhs, CameraMovement rhs) { return static_cast<u32>(lhs) & static_cast<u32>(rhs); }

class Camera {
public:
    Camera(f32 speed = 1.0f, f32 sensitivity = 1.0f);
    virtual ~Camera() = default;

    virtual void Update(f32 delta_time) = 0;
    virtual math::Mat4 GetViewProjectionMatrix() const = 0;

    void SetMovementFlag(CameraMovement flag);
    void ClearMovementFlag(CameraMovement flag);
    void ClearAllFlags() { m_movement_flags = CameraMovement::NONE; }

    void SetPosition(const math::Vec3 &pos)
    {
        m_position = pos;
        m_is_dirty = true; // Flag to recompute the view matrix
    }
    void SetRotation(const math::Vec2 &rot)
    {
        m_rotation = rot;
        m_is_dirty = true;
    }

    void Shake(f32 intensity, f32 duration);
    void SetSway(f32 amplitude, f32 frequency);
    void Follow(const math::Vec3 *target, f32 speed);

    math::Vec3 GetPosition() const { return m_position; }

protected:
    math::Vec3 ApplyEffects(f32 delta_time);
    void ApplyFollow(f32 delta_time);

protected:
    math::Vec3 m_position;
    math::Vec3 m_offset; // Temporary offset for effects
    math::Vec2 m_rotation;

    f32 m_speed;
    f32 m_sensitivity;

    CameraMovement m_movement_flags;

    mutable math::Mat4 m_view_projection_matrix;

    mutable bool m_is_dirty;

    // Shake parameters
    f32 m_shake_intensity;
    f32 m_shake_duration;

    // Sway parameters
    f32 m_sway_amplitude;
    f32 m_sway_frequency;
    f32 m_sway_time;

    // Follow parameters
    const math::Vec3 *p_follow_target;
    f32 m_follow_speed;

private:
    math::ThreadSafeRNG m_rng;
};

} // namespace gouda
