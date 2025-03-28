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
 * @copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
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
class OrthographicCamera : public Camera {
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
    OrthographicCamera(f32 left, f32 right, f32 bottom, f32 top, f32 zoom = 1.0f, f32 speed = 1.0f,
                       f32 sensitivity = 1.0f);

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
     * @brief Retrieves the current view-projection matrix.
     *
     * This method returns the camera's view-projection matrix, which combines the camera's view and projection
     * matrices into one matrix. It is used for transforming the 3D world coordinates to 2D screen space.
     *
     * @return The view-projection matrix of the camera.
     */
    Mat4 GetViewProjectionMatrix() const override;

    void SetProjection(f32 left, f32 right, f32 bottom, f32 top)
    {
        m_left = left;
        m_right = right;
        m_bottom = bottom;
        m_top = top;
        m_base_projection = math::ortho(left, right, bottom, top, -1.0f, 1.0f);

        m_is_dirty = true;
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

    f32 m_zoom; ///< The current zoom level of the camera.

    Mat4 m_base_projection; ///< The base projection matrix for the orthographic view.
};

} // namespace gouda
