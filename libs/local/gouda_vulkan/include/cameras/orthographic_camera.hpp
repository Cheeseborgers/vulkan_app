#pragma once

/**
 * @file cameras/orthographic_camera.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-12
 *
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "cameras/camera.hpp"

#include "math/math.hpp"

namespace gouda {

class OrthographicCamera : public Camera {
public:
    OrthographicCamera(f32 left, f32 right, f32 bottom, f32 top, f32 zoom = 1.0f, f32 speed = 1.0f,
                       f32 sensitivity = 1.0f);

    void Update(f32 delta_time) override;

    void AdjustZoom(float delta);

    math::Mat4 GetViewProjectionMatrix() const override;

private:
    void UpdateMatrix() const;

private:
    f32 m_left;
    f32 m_right;
    f32 m_bottom;
    f32 m_top;

    f32 m_zoom;

    math::Mat4 m_base_projection;
};

} // namespace gouda