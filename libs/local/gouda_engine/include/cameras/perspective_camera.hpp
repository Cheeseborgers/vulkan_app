#pragma once
/**
 * @file perspective_camera.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-12
 * @brief Perspective camera class for 3D rendering.
 *
 * This file defines the `PerspectiveCamera` class, which extends the base `Camera` class to implement a
 * perspective camera system. This camera type is commonly used in 3D games and simulations, where objects
 * are rendered with perspective distortion, meaning they appear smaller as they get further from the camera.
 * The class includes functionality for updating the camera's position, adjusting the field of view (FOV),
 * and generating the corresponding view-projection matrix.
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

namespace gouda {

/**
 * @class PerspectiveCamera
 * @brief A camera class that implements perspective projection.
 *
 * The `PerspectiveCamera` class extends the `Camera` class to provide a perspective view of the scene,
 * which is commonly used in 3D environments. Objects will appear smaller as they get farther from the camera,
 * creating the illusion of depth. The camera also allows for adjusting the field of view (FOV), which controls
 * the angle of the camera's view, affecting how much of the scene is visible.
 *
 * p_ortho_camera = std::make_unique<gouda::PerspectiveCamera>(60.0f, 1.777f, 0.1f, 1000.0f);
 * p_ortho_camera->SetPosition({0.0f, 0.0f, 5.0f});
 * p_ortho_camera->SetRotation(glm::vec2(0.0f, 3.1415926535f));
 *
 */
class PerspectiveCamera : public Camera {
public:
    /**
     * @brief Constructs a PerspectiveCamera object.
     *
     * Initializes the camera with the specified field of view (FOV), aspect ratio, near and far planes,
     * along with optional speed and sensitivity settings. The camera is set up for perspective rendering, where
     * objects are distorted based on their distance from the camera.
     *
     * @param fov The field of view (in degrees) of the camera.
     * @param aspect The aspect ratio (width/height) of the camera's view.
     * @param near The near clipping plane distance.
     * @param far The far clipping plane distance.
     * @param speed The speed of camera movement (default is 1.0f).
     * @param sensitivity The sensitivity of camera rotations (default is 1.0f).
     */
    PerspectiveCamera(f32 fov, f32 aspect, f32 near, f32 far, f32 speed = 1.0f, f32 sensitivity = 1.0f);

    /**
     * @brief Updates the camera's state for the current frame.
     *
     * This method is responsible for updating the camera's position, rotation, and other states. It should
     * be called every frame to keep the camera's state consistent. For a perspective camera, this includes
     * adjusting its position and view if needed.
     *
     * @param delta_time The time elapsed since the last frame (in seconds).
     */
    void Update(f32 delta_time) override;

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
     * matrices into one matrix. It is used for transforming the 3D world coordinates to 2D screen space, while
     * maintaining perspective distortion.
     *
     * @return The view-projection matrix of the camera.
     */
    Mat4 GetViewProjectionMatrix() const override;

    /**
     * @brief Sets the field of view (FOV) of the camera.
     *
     * This method allows you to change the camera's FOV, which controls how much of the scene is visible.
     * A larger FOV allows for a wider view, while a smaller FOV narrows the camera's perspective.
     *
     * @param fov The new field of view (in degrees).
     */
    void SetFOV(f32 fov);

private:
    /**
     * @brief Updates the camera's projection matrix.
     *
     * This method recalculates the camera's projection matrix based on the current FOV, aspect ratio, near and far
     * planes. It is used internally when the camera's projection needs to be updated.
     */
    void UpdateMatrix() const;

private:
    f32 m_fov;    ///< The field of view (in degrees) of the camera.
    f32 m_aspect; ///< The aspect ratio (width/height) of the camera's view.
    f32 m_near;   ///< The near clipping plane distance.
    f32 m_far;    ///< The far clipping plane distance.
};

} // namespace gouda