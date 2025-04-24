#pragma once
/**
 * @file math/quaternion.hpp
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
#include <cmath>

#include "core/types.hpp"
#include "math/matrix4x4.hpp"
#include "math/vector.hpp"

namespace gouda::math {

template <NumericT T>
class Quaternion {
public:
    T w, x, y, z;

    Quaternion(T w = T(1), T x = T(0), T y = T(0), T z = T(0)) : w(w), x(x), y(y), z(z) {}

    // Multiplication
    Quaternion operator*(const Quaternion &other) const
    {
        return Quaternion(w * other.w - x * other.x - y * other.y - z * other.z,
                          w * other.x + x * other.w + y * other.z - z * other.y,
                          w * other.y - x * other.z + y * other.w + z * other.x,
                          w * other.z + x * other.y - y * other.x + z * other.w);
    }

    // Convert to Matrix4x4
    Matrix4x4<T> ToMatrix() const
    {
        Matrix4x4<T> m;
        T xx = x * x, yy = y * y, zz = z * z;
        T xy = x * y, xz = x * z, yz = y * z;
        T wx = w * x, wy = w * y, wz = w * z;

        m(0, 0) = 1 - 2 * (yy + zz);
        m(0, 1) = 2 * (xy - wz);
        m(0, 2) = 2 * (xz + wy);
        m(1, 0) = 2 * (xy + wz);
        m(1, 1) = 1 - 2 * (xx + zz);
        m(1, 2) = 2 * (yz - wx);
        m(2, 0) = 2 * (xz - wy);
        m(2, 1) = 2 * (yz + wx);
        m(2, 2) = 1 - 2 * (xx + yy);
        return m;
    }

    // From axis-angle
    static Quaternion FromAxisAngle(const Vector<T, 3> &axis, T angle)
    {
        T s = std::sin(angle / 2);
        return Quaternion(std::cos(angle / 2), axis[0] * s, axis[1] * s, axis[2] * s);
    }
};

} // namespace gouda::math