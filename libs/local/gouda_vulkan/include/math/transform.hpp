#pragma once
/**
 * @file math/transform.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-13
 * @brief Engine module
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
#include "math/matrix4x4.hpp"
#include "math/quaternion.hpp"
#include "math/vector.hpp"

namespace gouda::math {

template <NumericT T>
class Transform {
public:
    Vector<T, 3> position;
    Quaternion<T> rotation;
    Vector<T, 3> scale{T(1), T(1), T(1)};

    Transform() = default;

    Matrix4x4<T> ToMatrix() const
    {
        Matrix4x4<T> t = Matrix4x4<T>::Translation(position);
        Matrix4x4<T> r = rotation.ToMatrix();
        Matrix4x4<T> s = Matrix4x4<T>::Scaling(scale);
        return t * r * s; // Translation * Rotation * Scale
    }
};

} // namespace gouda::math