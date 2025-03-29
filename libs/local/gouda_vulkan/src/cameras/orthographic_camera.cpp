/**
 * @file cameras/orthographic_camera.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-12
 * @brief A 2D orthographic camera that supports zoom and effects.
 */
#include "cameras/orthographic_camera.hpp"

#include "math/matrix4x4.hpp"
#include "math/vector.hpp"

#include "debug/logger.hpp"

namespace gouda {

OrthographicCamera::OrthographicCamera(f32 left, f32 right, f32 bottom, f32 top, f32 zoom, f32 speed, f32 sensitivity)
    : Camera{speed, sensitivity},
      m_left{left},
      m_right{right},
      m_bottom{bottom},
      m_top{top},
      m_zoom{zoom},
      m_base_projection{math::ortho(left, right, bottom, top, -1.0f, 1.0f)}
{
    UpdateMatrix();
}

void OrthographicCamera::Update(f32 delta_time)
{
    // Apply smooth follow if active
    ApplyFollow(delta_time);

    // Handle movement flags
    if (m_movement_flags != CameraMovement::NONE) {
        Vec2 movement;
        f32 zoom_delta{0.0f};

        if (m_movement_flags & CameraMovement::MOVE_LEFT)
            movement.x -= m_speed * delta_time;
        if (m_movement_flags & CameraMovement::MOVE_RIGHT)
            movement.x += m_speed * delta_time;
        if (m_movement_flags & CameraMovement::MOVE_UP)
            movement.y += m_speed * delta_time;
        if (m_movement_flags & CameraMovement::MOVE_DOWN)
            movement.y -= m_speed * delta_time;
        if (m_movement_flags & CameraMovement::ZOOM_IN)
            zoom_delta += m_sensitivity * delta_time;
        if (m_movement_flags & CameraMovement::ZOOM_OUT)
            zoom_delta -= m_sensitivity * delta_time;

        m_position.x += movement.x;
        m_position.y += movement.y;
        m_zoom = math::max(0.1f, m_zoom + zoom_delta);
        m_is_dirty = true;
    }

    // Apply shake and sway effects
    m_offset = ApplyEffects(delta_time); // Store offset

    // Set m_is_dirty if effects are active
    if (m_shake_duration > 0.0f || m_sway_amplitude > 0.0f) {
        m_is_dirty = true;
    }
}

void OrthographicCamera::AdjustZoom(float delta)
{
    m_zoom += delta;
    m_zoom = math::max(0.1f, m_zoom); // Prevent zoom from going too small
    m_is_dirty = true;                // Flag to update the projection matrix if needed
}

Mat4 OrthographicCamera::GetViewProjectionMatrix() const
{
    if (m_is_dirty) {
        UpdateMatrix();
    }

    return m_view_projection_matrix;
}

// Private functions ---------------------------------------------------------------------
void OrthographicCamera::UpdateMatrix() const
{
    // Calculate the total camera position from the offset
    Vec3 camera_position = m_position + m_offset;

    // Create the view matrix by translating the world opposite to the camera's position
    Mat4 view = math::translate(-(camera_position));

    // Create the projection matrix with zoom scaling
    Mat4 projection = m_base_projection * math::scale(Vec3(m_zoom, m_zoom, 1.0f));

    // Combine projection and view matrices
    m_view_projection_matrix = projection * view;

    // Mark as updated
    m_is_dirty = false;
}

} // namespace gouda