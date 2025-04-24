#pragma once
/**
 * @file camera/camera.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-12
 * @brief Camera system for 3D rendering with movement, shake, sway, and follow functionalities.
 *
 * This file contains the `Camera` class and associated types for handling camera movements, rotations,
 * and special effects (such as shake and sway). The camera class provides functionality for managing
 * 3D camera transformations, controlling movement, and supporting camera effects.
 *
 * The camera can also follow a target and has adjustable movement speed and sensitivity.
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

#include "core/types.hpp"
#include "math/math.hpp"
#include "math/random.hpp"

namespace gouda {

/**
 * @enum CameraMovement
 * @brief Flags representing different camera movement actions.
 *
 * Defines the movement actions that a camera can perform, such as moving left,
 * right, up, down, zooming in or out, and rotating along the X or Y axis for 3D cameras.
 */
enum class CameraMovement : u32 {
    NONE = 0,            ///< No movement
    MOVE_LEFT = 1 << 0,  ///< Move the camera left
    MOVE_RIGHT = 1 << 1, ///< Move the camera right
    MOVE_UP = 1 << 2,    ///< Move the camera up
    MOVE_DOWN = 1 << 3,  ///< Move the camera down
    ZOOM_IN = 1 << 4,    ///< Zoom the camera in
    ZOOM_OUT = 1 << 5,   ///< Zoom the camera out
    ROTATE_X = 1 << 6,   ///< Rotate the camera along the X axis (for 3D)
    ROTATE_Y = 1 << 7,   ///< Rotate the camera along the Y axis (for 3D)
};

/**
 * @brief Bitwise OR operator for combining camera movement flags.
 *
 * @param lhs The left-hand side camera movement flag.
 * @param rhs The right-hand side camera movement flag.
 * @return A combined camera movement flag.
 */
inline CameraMovement operator|(CameraMovement lhs, CameraMovement rhs)
{
    return static_cast<CameraMovement>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
}

/**
 * @brief Bitwise OR assignment operator for combining camera movement flags.
 *
 * @param lhs The left-hand side camera movement flag.
 * @param rhs The right-hand side camera movement flag.
 * @return The updated left-hand side camera movement flag.
 */
inline CameraMovement &operator|=(CameraMovement &lhs, CameraMovement rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

/**
 * @brief Bitwise AND operator for checking if two camera movement flags are set.
 *
 * @param lhs The left-hand side camera movement flag.
 * @param rhs The right-hand side camera movement flag.
 * @return True if both flags are set, false otherwise.
 */
inline bool operator&(CameraMovement lhs, CameraMovement rhs) { return static_cast<u32>(lhs) & static_cast<u32>(rhs); }

/**
 * @class Camera
 * @brief A class that represents a 3D camera with movement, effects, and follow capabilities.
 *
 * The Camera class handles the camera's position, rotation, movement, and special effects such as shaking and swaying.
 * It supports user-defined speed and sensitivity settings for controlling camera movement.
 */
class Camera {
public:
    /**
     * @brief Constructs a Camera object.
     *
     * @param speed The speed of camera movement.
     * @param sensitivity The sensitivity of camera rotations.
     */
    Camera(f32 speed = 1.0f, f32 sensitivity = 1.0f);

    /// Virtual destructor
    virtual ~Camera() = default;

    /**
     * @brief Updates the camera's state, typically called every frame.
     *
     * @param delta_time The time elapsed since the last update.
     */
    virtual void Update(f32 delta_time) = 0;

    /**
     * @brief Retrieves the current view matrix.
     *
     * @return The view matrix of the camera.
     */
    virtual Mat4 GetViewMatrix() const = 0;

    /**
     * @brief Retrieves the view-projection matrix of the camera.
     *
     * @return The view-projection matrix.
     */
    virtual Mat4 GetViewProjectionMatrix() const = 0;

    /**
     * @brief Sets the movement flag for the camera.
     *
     * @param flag The camera movement flag to set.
     */
    void SetMovementFlag(CameraMovement flag);

    /**
     * @brief Clears the specified movement flag for the camera.
     *
     * @param flag The camera movement flag to clear.
     */
    void ClearMovementFlag(CameraMovement flag);

    /**
     * @brief Clears all movement flags, resetting the camera's movement state.
     */
    void ClearAllFlags() { m_movement_flags = CameraMovement::NONE; }

    /**
     * @brief Sets the position of the camera.
     *
     * @param pos The new position of the camera.
     */
    void SetPosition(const Vec3 &pos);

    /**
     * @brief Sets the rotation of the camera.
     *
     * @param rot The new rotation of the camera in 2D (pitch, yaw).
     */
    void SetRotation(const Vec2 &rot);

    /**
     * @brief Applies shake effects to the camera.
     *
     * @param intensity The intensity of the shake.
     * @param duration The duration of the shake effect.
     */
    void Shake(f32 intensity, f32 duration);

    /**
     * @brief Sets the sway effect parameters for the camera.
     *
     * @param amplitude The amplitude of the sway.
     * @param frequency The frequency of the sway.
     */
    void SetSway(f32 amplitude, f32 frequency);

    /**
     * @brief Makes the camera follow a target.
     *
     * @param target The target position the camera should follow.
     * @param speed The speed at which the camera follows the target.
     */
    void Follow(const Vec3 *target, f32 speed);

    /**
     * @brief Retrieves the current position of the camera.
     *
     * @return The camera's position.
     */
    Vec3 GetPosition() const { return m_position; }

    bool IsDirty() const { return m_is_dirty; }

protected:
    /**
     * @brief Applies the various camera effects (e.g., shake, sway).
     *
     * @param delta_time The time elapsed since the last update.
     * @return The camera's new position after applying effects.
     */
    Vec3 ApplyEffects(f32 delta_time);

    /**
     * @brief Applies the follow effect for the camera to track the target.
     *
     * @param delta_time The time elapsed since the last update.
     */
    void ApplyFollow(f32 delta_time);

protected:
    Vec3 m_position; ///< The position of the camera.
    Vec3 m_offset;   ///< Temporary offset for shake and sway effects.
    Vec2 m_rotation; ///< The rotation of the camera (pitch, yaw).

    f32 m_speed;       ///< Movement speed of the camera.
    f32 m_sensitivity; ///< Sensitivity for the camera's rotations.

    CameraMovement m_movement_flags; ///< Flags for camera movement actions.

    mutable Mat4 m_view_projection_matrix; ///< Cached view-projection matrix.
    mutable Mat4 m_view_matrix;            ///< Cached view matrix.
    mutable bool m_is_dirty;               ///< Flag indicating if the view matrix needs to be recomputed.

    // Shake effect parameters
    f32 m_shake_intensity; ///< Intensity of the shake effect.
    f32 m_shake_duration;  ///< Duration of the shake effect.

    // Sway effect parameters
    f32 m_sway_amplitude; ///< Amplitude of the sway effect.
    f32 m_sway_frequency; ///< Frequency of the sway effect.
    f32 m_sway_time;      ///< Time tracking for sway animation.

    // Follow effect parameters
    const Vec3 *p_follow_target; ///< Target that the camera should follow.
    f32 m_follow_speed;          ///< Speed at which the camera follows the target.

private:
    math::ThreadSafeRNG m_rng; ///< Thread-safe random number generator for effects.
};

} // namespace gouda
