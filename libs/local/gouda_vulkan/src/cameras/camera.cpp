/**
 * @file camera.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-13
 * @brief Engine camera base implementation
 */
#include "cameras/camera.hpp"

#include "logger.hpp"

namespace gouda {

Camera::Camera(f32 speed, f32 sensitivity)
    : m_position{},
      m_offset{},
      m_rotation{},
      m_speed{speed},
      m_sensitivity{sensitivity},
      m_movement_flags(CameraMovement::NONE),
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

void Camera::Shake(f32 intensity, f32 duration)
{
    m_shake_intensity = intensity;
    m_shake_duration = duration;
}

void Camera::SetSway(f32 amplitude, f32 frequency)
{
    m_sway_amplitude = amplitude;
    m_sway_frequency = frequency;
}

void Camera::Follow(const math::Vec3 *target, f32 speed)
{
    p_follow_target = target;
    m_follow_speed = speed;
}

// Protected functions -------------------------------------------------------------------------------
math::Vec3 Camera::ApplyEffects(f32 delta_time)
{
    math::Vec3 offset{};

    // Apply shake if active
    if (m_shake_duration > 0.0f) {
        offset.x += m_rng.GetFloat(-m_shake_intensity, m_shake_intensity);
        offset.y += m_rng.GetFloat(-m_shake_intensity, m_shake_intensity);
        m_shake_duration -= delta_time;
    }

    // Apply sway if active
    if (m_sway_amplitude > 0.0f) {
        m_sway_time += delta_time * m_sway_frequency;
        offset.y += sin(m_sway_time) * m_sway_amplitude;
    }

    return offset;
}

void Camera::ApplyFollow(f32 delta_time)
{
    if (p_follow_target && m_follow_speed > 0.0f) {
        m_position += (*p_follow_target - m_position) * m_follow_speed * delta_time;
        m_is_dirty = true;
    }
}

} // namespace gouda