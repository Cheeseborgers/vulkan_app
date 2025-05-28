#pragma once
/**
 * @file cameras/orthographic_camera.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-12
 * @brief Orthographic camera class for 2D rendering.
 *
 * This file defines the `OrthographicCamera` class, which extends the base `Camera` class to implement an
 * orthographic camera system. This camera type is typically used in 2D games or scenes where perspective
 * distortion is not needed, and objects are rendered with a constant size regardless of their distance from the camera.
 * It supports zooming and adjusting the view's projection matrix.
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */

#include "cameras/camera.hpp"
#include "math/math.hpp"

namespace gouda {

/**
 * @class OrthographicCamera
 * @brief A camera class that implements orthographic projection.
 *
 * The `OrthographicCamera` class extends the `Camera` class to provide an orthographic view of the scene,
 * which is ideal for 2D rendering. Unlike perspective cameras, objects maintain a constant size regardless
 * of their distance from the camera. This class includes functionality for adjusting the zoom level
 * and generating the corresponding view-projection matrix.
 */
class OrthographicCamera final : public Camera {
public:
    /**
     * @brief Constructs an OrthographicCamera object.
     *
     * Initializes the camera with the specified bounds for the orthographic view, the zoom level,
     * and optional speed and sensitivity settings. The zoom factor modifies the overall scale of the view.
     *
     * @param left The left boundary of the orthographic view.
     * @param right The right boundary of the orthographic view.
     * @param bottom The bottom boundary of the orthographic view.
     * @param top The top boundary of the orthographic view.
     * @param zoom The initial zoom level of the camera (default is 1.0f).
     * @param speed The speed of camera movement (default is 1.0f).
     * @param sensitivity The sensitivity of camera rotations (default is 1.0f).
     */
    OrthographicCamera(f32 left, f32 right, f32 bottom, f32 top, f32 near = -1.0f, f32 far = 1.0f, f32 zoom = 1.0f,
                       f32 speed = 1.0f, f32 sensitivity = 1.0f);

    /**
     * @brief Updates the camera's state for the current frame.
     *
     * This method is responsible for updating the camera's position, rotation, and other states. It should
     * be called every frame to keep the camera's state consistent. For an orthographic camera, this includes
     * adjusting its position and view if needed.
     *
     * @param delta_time The time elapsed since the last frame (in seconds).
     */
    void Update(f32 delta_time) override;

    /**
     * @brief Adjusts the zoom level of the camera.
     *
     * This method allows zooming in or out by modifying the camera's zoom factor. A zoom factor greater than
     * 1.0f will zoom in, while a value less than 1.0f will zoom out.
     *
     * @param delta The amount to change the zoom factor. Positive values zoom in, negative values zoom out.
     */
    void AdjustZoom(float delta);

    /**
     * @brief Retrieves the current view matrix.
     *
     * This method returns the camera's view matrix
     *
     * @return The view matrix of the camera.
     */
    Mat4 GetViewMatrix() const override { return m_view_matrix; }

    /**
     * @brief Retrieves the current view-projection matrix.
     *
     * This method returns the camera's view-projection matrix, which combines the camera's view and projection
     * matrices into one matrix. It is used for transforming the 3D world coordinates to 2D screen space.
     *
     * @return The view-projection matrix of the camera.
     */
    Mat4 GetViewProjectionMatrix() const override;

    f32 GetLeft() const { return m_left / m_zoom; }
    f32 GetRight() const { return m_right / m_zoom; }
    f32 GetBottom() const { return m_bottom / m_zoom; }
    f32 GetTop() const { return m_top / m_zoom; }
    f32 GetNear() const { return m_near; }
    f32 GetFar() const { return m_far; }
    f32 GetZoom() const { return m_zoom; }
    Vec3 GetPosition() const override { return m_position; }

    void SetProjection(const f32 left, const f32 right, const f32 bottom, const f32 top)
    {
        m_left = left;
        m_right = right;
        m_bottom = bottom;
        m_top = top;
        m_base_projection = math::ortho(left, right, bottom, top, m_near, m_far);
        m_is_dirty = true;
    }

    struct FrustumData {
        Vec3 position; ///< Camera position in world space
        f32 left;      ///< The left boundary of the orthographic view, adjusted for zoom
        f32 right;     ///< The right boundary of the orthographic view, adjusted for zoom
        f32 bottom;    ///< The bottom boundary of the orthographic view, adjusted for zoom
        f32 top;       ///< The top boundary of the orthographic view, adjusted for zoom
    };

    FrustumData GetFrustumData() const
    {
        FrustumData data{};
        data.position = m_position;
        data.left = m_left / m_zoom;
        data.right = m_right / m_zoom;
        data.bottom = m_bottom / m_zoom;
        data.top = m_top / m_zoom;

        return data;
    }

private:
    /**
     * @brief Updates the camera's projection matrix.
     *
     * This method recalculates the camera's projection matrix based on the current bounds (left, right, bottom, top)
     * and zoom level. It is used internally when the camera's projection needs to be updated.
     */
    void UpdateMatrix() const;

private:
    f32 m_left;   ///< The left boundary of the orthographic view.
    f32 m_right;  ///< The right boundary of the orthographic view.
    f32 m_bottom; ///< The bottom boundary of the orthographic view.
    f32 m_top;    ///< The top boundary of the orthographic view.

    f32 m_near;
    f32 m_far;

    f32 m_zoom; ///< The current zoom level

    Mat4 m_base_projection; ///< The base projection matrix for the orthographic view.
};

} // namespace gouda
