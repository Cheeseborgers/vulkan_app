#pragma once
/**
 * @file math/math.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-12
 * @brief Engine math functions
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
#include "math/vector.hpp"

namespace gouda {
namespace math {

// Type defs for Vector and Matrix types
using Vec2 = Vector<float, 2>;
using Vec3 = Vector<float, 3>;
using Mat4 = Matrix4x4<float>;

constexpr u32 bit_shift(int x) { return 1U << x; }
constexpr u64 bytes_to_kb(u64 bytes) { return bytes / 1024ULL; }
constexpr u64 bytes_to_mb(u64 bytes) { return bytes / (1024ULL * 1024); }
constexpr u64 bytes_to_gb(u64 bytes) { return bytes / (1024ULL * 1024 * 1024); }

// Function that returns π with the correct precision
template <FloatingPointT T>
constexpr T pi()
{
    if constexpr (std::is_same_v<T, float>) {
        return 3.1415927f; // 7 decimal places
    }
    else {
        return 3.141592653589793; // 15 decimal places (for double & long double)
    }
}

/**
 * @brief Returns the largest possible value for type T.
 * @tparam T The data type.
 * @return The maximum value representable by type T.
 */
template <NumericT T>
constexpr auto max_value() -> T
{
    return std::numeric_limits<T>::max();
}

/**
 * @brief Calculates the maximum of two values.
 * @tparam T The data type of the values.
 * @param a The first value.
 * @param b The second value.
 * @return The greater of the two input values.
 */
template <NumericT T>
constexpr T max(T a, T b)
{
    return a > b ? a : b;
}

/**
 * @brief Calculates the minimum of two values.
 * @tparam T The data type of the values.
 * @param a The first value.
 * @param b The second value.
 * @return The smaller of the two input values.
 */
template <NumericT T>
constexpr T min(T a, T b)
{
    return a < b ? a : b;
}

/**
 * @brief Calculates the maximum value among a list of values.
 * @tparam T The data type of the values.
 * @param values The list of values.
 * @return The larger value among the input values.
 */
template <NumericT T>
constexpr T max(std::initializer_list<T> values)
{
    T max_value = *values.begin();
    for (const T &value : values) {
        if (value > max_value) {
            max_value = value;
        }
    }
    return max_value;
}
/**
 * @brief Calculates the minimum value among a list of values.
 * @tparam T The data type of the values.
 * @param values The list of values.
 * @return The minimum value among the input values.
 */
template <NumericT T>
constexpr T min(std::initializer_list<T> values)
{
    T min_value = *values.begin();
    for (const T &value : values) {
        if (value < min_value) {
            min_value = value;
        }
    }
    return min_value;
}

template <NumericT T>
constexpr T clamp(const T &value, const T &min_value, const T &max_value)
{
    if (value < min_value) {
        return min_value;
    }
    else if (value > max_value) {
        return max_value;
    }
    else {
        return value;
    }
}

// Degrees to radians
template <NumericT T>
constexpr T radians(T degrees)
{
    return degrees * static_cast<T>(0.01745329251994329576923690768489); // π / 180
}

// Radians to degrees
template <NumericT T>
constexpr T degrees(T radians)
{
    return radians * static_cast<T>(57.295779513082320876798154814105); // 180 / π
}

// Translation matrix
template <NumericT T>
Matrix4x4<T> translate(const Vector<T, 3> &translation)
{
    Matrix4x4<T> m;           // Identity matrix by default
    m(0, 3) = translation[0]; // x
    m(1, 3) = translation[1]; // y
    m(2, 3) = translation[2]; // z
    return m;
}

// Scaling matrix
template <NumericT T>
Matrix4x4<T> scale(const Vector<T, 3> &scale_factors)
{
    Matrix4x4<T> m;             // Identity matrix by default
    m(0, 0) = scale_factors[0]; // x scale
    m(1, 1) = scale_factors[1]; // y scale
    m(2, 2) = scale_factors[2]; // z scale
    return m;
}

// Orthographic projection matrix
template <NumericT T>
Matrix4x4<T> ortho(T left, T right, T bottom, T top, T near, T far)
{
    Matrix4x4<T> m;
    m(0, 0) = T(2) / (right - left);
    m(1, 1) = T(2) / (top - bottom);
    m(2, 2) = T(-2) / (far - near);
    m(0, 3) = -(right + left) / (right - left);
    m(1, 3) = -(top + bottom) / (top - bottom);
    m(2, 3) = -(far + near) / (far - near);
    m(3, 3) = T(1);
    return m;
}

// Perspective projection matrix (OpenGL-style, depth range [-1, 1])
template <NumericT T>
Matrix4x4<T> perspective(T fovy, T aspect, T near, T far)
{
    T tanHalfFovy = std::tan(fovy / T(2));
    Matrix4x4<T> m;
    m(0, 0) = T(1) / (aspect * tanHalfFovy);
    m(1, 1) = T(1) / tanHalfFovy;
    m(2, 2) = -(far + near) / (far - near);
    m(2, 3) = T(-2) * far * near / (far - near);
    m(3, 2) = T(-1); // Perspective divide
    m(3, 3) = T(0);
    return m;
}

// Look-at view matrix
template <NumericT T>
Matrix4x4<T> lookAt(const Vector<T, 3> &eye, const Vector<T, 3> &center, const Vector<T, 3> &up)
{
    Vector<T, 3> forward = (center - eye).Normalized();
    Vector<T, 3> right = forward.Cross(up).Normalized();
    Vector<T, 3> up_adjusted = right.Cross(forward);

    Matrix4x4<T> m;
    m(0, 0) = right[0];
    m(0, 1) = right[1];
    m(0, 2) = right[2];
    m(1, 0) = up_adjusted[0];
    m(1, 1) = up_adjusted[1];
    m(1, 2) = up_adjusted[2];
    m(2, 0) = -forward[0];
    m(2, 1) = -forward[1];
    m(2, 2) = -forward[2];
    m(0, 3) = -right.Dot(eye);
    m(1, 3) = -up_adjusted.Dot(eye);
    m(2, 3) = forward.Dot(eye);
    m(3, 3) = T(1);
    return m;
}

// Rotation matrix around an axis
template <NumericT T>
Matrix4x4<T> rotate(const Vector<T, 3> &axis, T angle)
{
    Vector<T, 3> n = axis.Normalized();
    T c = std::cos(angle);
    T s = std::sin(angle);
    T t = T(1) - c;

    Matrix4x4<T> m;
    m(0, 0) = c + n[0] * n[0] * t;
    m(0, 1) = n[0] * n[1] * t - n[2] * s;
    m(0, 2) = n[0] * n[2] * t + n[1] * s;
    m(1, 0) = n[1] * n[0] * t + n[2] * s;
    m(1, 1) = c + n[1] * n[1] * t;
    m(1, 2) = n[1] * n[2] * t - n[0] * s;
    m(2, 0) = n[2] * n[0] * t - n[1] * s;
    m(2, 1) = n[2] * n[1] * t + n[0] * s;
    m(2, 2) = c + n[2] * n[2] * t;
    m(3, 3) = T(1);
    return m;
}

// Transpose of a matrix
template <NumericT T>
Matrix4x4<T> transpose(const Matrix4x4<T> &m)
{
    Matrix4x4<T> result;
    for (size_t row = 0; row < 4; ++row) {
        for (size_t col = 0; col < 4; ++col) {
            result(row, col) = m(col, row);
        }
    }
    return result;
}

} // namespade math
} // namespace gouda