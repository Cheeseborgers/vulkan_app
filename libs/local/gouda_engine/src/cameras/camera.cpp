/**
 * @file camera.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-13
 * @brief Engine camera base implementation
 */
#include "cameras/camera.hpp"

#include "debug/logger.hpp"

namespace gouda {

Camera::Camera(const f32 speed, const f32 sensitivity)
    : m_speed{speed},
      m_sensitivity{sensitivity},
      m_movement_flags(CameraMovement::None),
      m_is_dirty{true},
      m_shake_intensity{0.0f},
      m_shake_duration{0.0f},
      m_sway_amplitude{0.0f},
      m_sway_frequency{0.0f},
      m_sway_time{0.0f},
      p_follow_target{nullptr},
      m_follow_speed{0.0f},
      m_rng{math::GenerateSeed()}
{
}

void Camera::SetMovementFlag(CameraMovement flag)
{
    m_movement_flags |= flag;
    m_is_dirty = true;
}

void Camera::ClearMovementFlag(CameraMovement flag)
{
    m_movement_flags = static_cast<CameraMovement>(static_cast<u32>(m_movement_flags) & ~static_cast<u32>(flag));
}

void Camera::SetPosition(const Vec3 &position)
{
    m_position = position;
    // m_is_dirty = true;
}

void Camera::SetRotation(const Vec2 &rotation)
{
    m_rotation = rotation;
    // m_is_dirty = true;
}

void Camera::Shake(const f32 intensity, const f32 duration)
{
    m_shake_intensity = intensity;
    m_shake_duration = duration;
}

void Camera::SetSway(const f32 amplitude, const f32 frequency)
{
    m_sway_amplitude = amplitude;
    m_sway_frequency = frequency;
}

void Camera::Follow(const Vec3 *target, const f32 speed)
{
    p_follow_target = target;
    m_follow_speed = speed;
}

// Protected functions -------------------------------------------------------------------------------
Vec3 Camera::ApplyEffects(const f32 delta_time)
{
    Vec3 offset{};

    // Apply shake if active
    if (m_shake_duration > 0.0f) {
        offset.x += m_rng.GetFloat(-m_shake_intensity, m_shake_intensity);
        offset.y += m_rng.GetFloat(-m_shake_intensity, m_shake_intensity);
        m_shake_duration -= delta_time;
    }

    // Apply sway if active
    if (m_sway_amplitude > 0.0f) {
        m_sway_time += delta_time * m_sway_frequency;
        offset.y += static_cast<f32>(sin(m_sway_time) * m_sway_amplitude);
    }

    return offset;
}

void Camera::ApplyFollow(const f32 delta_time)
{
    if (p_follow_target && m_follow_speed > 0.0f) {
        m_position += (*p_follow_target - m_position) * m_follow_speed * delta_time;
        m_is_dirty = true;
    }
}

} // namespace gouda