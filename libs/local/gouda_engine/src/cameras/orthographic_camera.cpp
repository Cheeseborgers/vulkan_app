/**
 * @file orthographic_camera.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-12
 * @brief A 2D orthographic camera that supports zoom and effects.
 */
#include "cameras/orthographic_camera.hpp"

#include "math/matrix4x4.hpp"
#include "math/vector.hpp"

#include "debug/logger.hpp"

namespace gouda {

OrthographicCamera::OrthographicCamera(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far, f32 zoom, f32 speed,
                                       f32 sensitivity)
    : Camera{speed, sensitivity},
      m_left{left},
      m_right{right},
      m_bottom{bottom},
      m_top{top},
      m_near{near},
      m_far{far},
      m_zoom{zoom},
      m_base_projection{math::ortho(left, right, bottom, top, near, far)}
{
    UpdateMatrix();
}

void OrthographicCamera::Update(const f32 delta_time)
{
    if (m_speed == 0.0f && m_sensitivity == 0.0f) {
        m_is_dirty = false;
        return;
    }

    // Apply smooth follow if active
    ApplyFollow(delta_time);

    // Handle movement flags
    if (m_movement_flags != CameraMovement::None) {
        Vec2 movement{0.0f};
        f32 zoom_delta{0.0f};

        if (m_movement_flags & CameraMovement::MoveLeft)
            movement.x -= m_speed * delta_time;
        if (m_movement_flags & CameraMovement::MoveRight)
            movement.x += m_speed * delta_time;
        if (m_movement_flags & CameraMovement::MoveUp)
            movement.y += m_speed * delta_time;
        if (m_movement_flags & CameraMovement::MoveDown)
            movement.y -= m_speed * delta_time;
        if (m_movement_flags & CameraMovement::ZoomIn)
            zoom_delta += m_sensitivity * delta_time;
        if (m_movement_flags & CameraMovement::ZoomOut)
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
    m_is_dirty = true;
}

Mat4 OrthographicCamera::GetViewProjectionMatrix() const
{
    if (m_is_dirty) {
        UpdateMatrix();
    }

    return m_view_projection_matrix;
}

Mat4 OrthographicCamera::GetViewMatrixNoEffects() const
{
    //if (m_speed == 0.0f && m_sensitivity == 0.0f) {
        //return Mat4::identity();
    //}
    return math::translate(-m_position);
}

Mat4 OrthographicCamera::GetBaseProjection() const
{
    return m_base_projection;
}

// Private functions ---------------------------------------------------------------------
void OrthographicCamera::UpdateMatrix() const
{
    // Calculate the total camera position from the offset
    const Vec3 camera_position{m_position + m_offset};

    // Create the view matrix by translating the world opposite to the camera's position
    const Mat4 view_matrix{math::translate(-camera_position)};

    // Create the projection matrix with zoom scaling
    const Mat4 projection{m_base_projection * math::scale(Vec3(m_zoom, m_zoom, 1.0f))};

    // Combine projection and view matrices
    m_view_projection_matrix = projection * view_matrix;

    // Mark as updated
    m_is_dirty = false;
}

} // namespace gouda