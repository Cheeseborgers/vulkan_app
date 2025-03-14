/**
 * @file perspective_camera.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-12
 * @brief A perspective camera for 3D scenes.
 */
#include "cameras/perspective_camera.hpp"

namespace gouda {

PerspectiveCamera::PerspectiveCamera(f32 fov, f32 aspect, f32 near, f32 far, f32 speed, f32 sensitivity)
    : Camera{speed, sensitivity}, m_fov{fov}, m_aspect{aspect}, m_near{near}, m_far{far}
{
    UpdateMatrix();
}

void PerspectiveCamera::Update(f32 delta_time)
{
    // Apply smooth follow if active
    ApplyFollow(delta_time);

    // Handle movement flags
    if (m_movement_flags != CameraMovement::NONE) {
        Vec3 movement{0.0f, 0.0f, 0.0f};
        Vec2 rotation_delta{0.0f, 0.0f};

        if (m_movement_flags & CameraMovement::MOVE_LEFT)
            movement.x -= m_speed * delta_time;
        if (m_movement_flags & CameraMovement::MOVE_RIGHT)
            movement.x += m_speed * delta_time;
        if (m_movement_flags & CameraMovement::MOVE_UP)
            movement.y += m_speed * delta_time;
        if (m_movement_flags & CameraMovement::MOVE_DOWN)
            movement.y -= m_speed * delta_time;
        if (m_movement_flags & CameraMovement::ROTATE_X)
            rotation_delta.x += m_sensitivity * delta_time;
        if (m_movement_flags & CameraMovement::ROTATE_Y)
            rotation_delta.y += m_sensitivity * delta_time;

        m_position += movement;
        m_rotation += rotation_delta;
        m_is_dirty = true;
    }

    // Apply shake and sway effects
    m_offset = ApplyEffects(delta_time); // Store offset

    // Set m_is_dirty if effects are active
    if (m_shake_duration > 0.0f || m_sway_amplitude > 0.0f) {
        m_is_dirty = true;
    }
}

Mat4 PerspectiveCamera::GetViewProjectionMatrix() const
{
    if (m_is_dirty) {
        UpdateMatrix();
    }

    return m_view_projection_matrix;
}

void PerspectiveCamera::SetFOV(f32 fov)
{
    m_fov = fov;
    m_is_dirty = true;
}

// Private functions ----------------------------------------------------------------------------------
void PerspectiveCamera::UpdateMatrix() const
{
    // Compute total position with offset
    Vec3 total_position{m_position + m_offset};

    // Compute forward direction from pitch (x) and yaw (y)
    f32 pitch{m_rotation.x};
    f32 yaw{m_rotation.y};
    Vec3 forward = Vec3(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));

    // Define up vector (world y-axis)
    Vec3 up{Vec3(0.0f, 1.0f, 0.0f)};

    // Construct view matrix using lookAt
    Mat4 view{math::lookAt(total_position, total_position + forward, up)};
    Mat4 projection{math::perspective(math::radians(m_fov), m_aspect, m_near, m_far)};
    m_view_projection_matrix = projection * view;
    m_is_dirty = false;
}

} // namespace gouda