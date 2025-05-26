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
#include <bit>

#include "math/matrix4x4.hpp"
#include "math/simd.hpp"
#include "math/vector.hpp"
#include "core/types.hpp"
#include "debug/assert.hpp"

namespace gouda {

// Type defs for Vector and Matrix types
using Vec2 = math::Vector<float, 2>;
using Vec3 = math::Vector<float, 3>;
using Vec4 = math::Vector<float, 4>;

using UVec2 = math::Vector<u_int32_t, 2>;
using UVec3 = math::Vector<u_int32_t, 3>;
using UVec4 = math::Vector<u_int32_t, 4>;

using Mat4 = math::Matrix4x4<float>;

namespace math {

// Bit manipulation using C++23 <bit>
constexpr u32 bit_shift(int x) noexcept { return std::bit_cast<u32>(1U << x); }

constexpr u64 bytes_to_kb(const u64 bytes) { return bytes / 1024ULL; }
constexpr u64 bytes_to_mb(const u64 bytes) { return bytes / (1024ULL * 1024); }
constexpr u64 bytes_to_gb(const u64 bytes) { return bytes / (1024ULL * 1024 * 1024); }


constexpr size_t mb_to_bytes(const size_t mb)
{
    if (mb > constants::size_t_max / constants::mb) {
        if consteval {
            ASSERT(false, "Error");
        }
        throw std::overflow_error("Overflow in mb_to_bytes");
    }
    return mb * constants::mb;
}

constexpr size_t gb_to_bytes(const size_t gb)
{
    if (gb > constants::size_t_max / constants::gb) {
        if consteval {
            ASSERT(false, "Error");
        }
        throw std::overflow_error("Overflow in gb_to_bytes");
    }
    return gb * constants::gb;
}

constexpr size_t set_size_in_mb(const size_t mb) { return mb_to_bytes(mb); }
constexpr size_t set_size_in_gb(const size_t gb) { return gb_to_bytes(gb); }

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

template <typename T>
[[nodiscard]] f32 aspect_ratio(const T &size) noexcept
{
    ASSERT(size.width != 0 && size.height != 0, "Division by zero error");
    return static_cast<f32>(size.width) / size.height;
}

// Clamp with SIMD optimization
template <NumericT T>
[[nodiscard]] T clamp(T value, T min_val, T max_val) noexcept
{
    return std::clamp(value, min_val, max_val);
}

template <FloatingPointT T>
[[nodiscard]] constexpr T radians(T degrees) noexcept
{
    return degrees * (pi<T>() / T(180));
}

template <FloatingPointT T>
[[nodiscard]] constexpr T degrees(T radians) noexcept
{
    return radians * (T(180) / pi<T>);
}

// Translation matrix
template <FloatingPointT T>
[[nodiscard]] Matrix4x4<T> translate(const Vector<T, 3> &t) noexcept
{
    Matrix4x4<T> m = Matrix4x4<T>::identity();

    if (std::is_same_v<T, float> && simdLevel >= SIMDLevel::AVX) {
        // Load translation components into last column
        const __m128 trans = _mm_set_ps(1.f, t[2], t[1], t[0]);
        _mm_storeu_ps(&m.data[12], trans); // Store directly into last column (12-15)
    }
    else if (std::is_same_v<T, float> && simdLevel >= SIMDLevel::SSE2) {
        const __m128 trans = _mm_set_ps(1.f, t[2], t[1], t[0]);
        _mm_storeu_ps(&m.data[12], trans);
    }
    else {
        m(0, 3) = t[0];
        m(1, 3) = t[1];
        m(2, 3) = t[2];
        // m(3, 3) remains 1 from identity
    }
    return m;
}

// Scaling matrix
template <FloatingPointT T>
[[nodiscard]] Matrix4x4<T> scale(const Vector<T, 3> &s) noexcept
{
    Matrix4x4<T> m = Matrix4x4<T>::identity();

    if (std::is_same_v<T, float> && simdLevel >= SIMDLevel::AVX) {
        // Set diagonal elements using SIMD
        const __m128 diag0 = _mm_set_ps(0.f, 0.f, 0.f, s[0]); // m[0][0]
        const __m128 diag1 = _mm_set_ps(0.f, 0.f, s[1], 0.f); // m[1][1]
        const __m128 diag2 = _mm_set_ps(0.f, s[2], 0.f, 0.f); // m[2][2]
        const __m128 diag3 = _mm_set_ps(1.f, 0.f, 0.f, 0.f);  // m[3][3]

        _mm_storeu_ps(&m.data[0], diag0);
        _mm_storeu_ps(&m.data[4], diag1);
        _mm_storeu_ps(&m.data[8], diag2);
        _mm_storeu_ps(&m.data[12], diag3);
    }
    else if (std::is_same_v<T, float> && simdLevel >= SIMDLevel::SSE2) {
        // Same as AVX but with SSE
        const __m128 diag0 = _mm_set_ps(0.f, 0.f, 0.f, s[0]);
        const __m128 diag1 = _mm_set_ps(0.f, 0.f, s[1], 0.f);
        const __m128 diag2 = _mm_set_ps(0.f, s[2], 0.f, 0.f);
        const __m128 diag3 = _mm_set_ps(1.f, 0.f, 0.f, 0.f);

        _mm_storeu_ps(&m.data[0], diag0);
        _mm_storeu_ps(&m.data[4], diag1);
        _mm_storeu_ps(&m.data[8], diag2);
        _mm_storeu_ps(&m.data[12], diag3);
    }
    else {
        m(0, 0) = s[0];
        m(1, 1) = s[1];
        m(2, 2) = s[2];
        // m(3, 3) remains 1 from identity
    }
    return m;
}

// Orthographic projection matrix
template <FloatingPointT T>
[[nodiscard]] Matrix4x4<T> ortho(T left, T right, T bottom, T top, T near, T far) noexcept
{
    Matrix4x4<T> m = Matrix4x4<T>::identity();

    const T rl_inv = T(1) / (right - left);
    const T tb_inv = T(1) / (top - bottom);
    const T fn_inv = T(1) / (far - near);

    m(0, 0) = T(2) * rl_inv;
    m(1, 1) = T(2) * tb_inv;
    m(2, 2) = T(-2) * fn_inv;
    m(0, 3) = -(right + left) * rl_inv;
    m(1, 3) = -(top + bottom) * tb_inv;
    m(2, 3) = -(far + near) * fn_inv;
    m(3, 3) = T(1);

    return m;
}

template <FloatingPointT T>
[[nodiscard]] Mat4 perspective(T fovy, T aspect, T near, T far) noexcept
{
    const T tan_half = std::tan(fovy * T(0.5));
    Matrix4x4<T> m = Matrix4x4<T>::identity();

    m(0, 0) = T(1) / (aspect * tan_half);
    m(1, 1) = T(1) / tan_half;
    m(2, 2) = -(far + near) / (far - near);
    m(2, 3) = T(-2) * far * near / (far - near);
    m(3, 2) = T(-1);

    return m;
}

template <FloatingPointT T>
[[nodiscard]] Matrix4x4<T> lookAt(const Vector<T, 3> &eye, const Vector<T, 3> &center,
                                         const Vector<T, 3> &up) noexcept
{
    const Vector<T, 3> forward = (center - eye).normalized();
    const Vector<T, 3> right = forward.cross(up).normalized();
    const Vector<T, 3> up_adjusted = right.cross(forward);

    Matrix4x4<T> m = Matrix4x4<T>::identity();

    if (std::is_same_v<T, float> && simdLevel >= SIMDLevel::AVX) {
        // Column-major storage: each __m128 is one column
        const __m128 col0 = _mm_set_ps(0.f, 0.f, 0.f, right[0]);
        const __m128 col1 = _mm_set_ps(0.f, 0.f, 0.f, right[1]);
        const __m128 col2 = _mm_set_ps(0.f, -forward[2], up_adjusted[2], right[2]);
        const __m128 col3 = _mm_set_ps(1.f, forward.dot(eye), -up_adjusted.dot(eye), -right.dot(eye));

        _mm_storeu_ps(&m.data[0], col0);
        _mm_storeu_ps(&m.data[4], col1);
        _mm_storeu_ps(&m.data[8], col2);
        _mm_storeu_ps(&m.data[12], col3);
    }
    else if (std::is_same_v<T, float> && simdLevel >= SIMDLevel::SSE2) {
        const __m128 col0 = _mm_set_ps(0.f, 0.f, 0.f, right[0]);
        const __m128 col1 = _mm_set_ps(0.f, 0.f, 0.f, right[1]);
        const __m128 col2 = _mm_set_ps(0.f, -forward[2], up_adjusted[2], right[2]);
        const __m128 col3 = _mm_set_ps(1.f, forward.dot(eye), -up_adjusted.dot(eye), -right.dot(eye));

        _mm_storeu_ps(&m.data[0], col0);
        _mm_storeu_ps(&m.data[4], col1);
        _mm_storeu_ps(&m.data[8], col2);
        _mm_storeu_ps(&m.data[12], col3);
    }
    else {
        m(0, 0) = right[0];
        m(0, 1) = right[1];
        m(0, 2) = right[2];
        m(1, 0) = up_adjusted[0];
        m(1, 1) = up_adjusted[1];
        m(1, 2) = up_adjusted[2];
        m(2, 0) = -forward[0];
        m(2, 1) = -forward[1];
        m(2, 2) = -forward[2];
        m(0, 3) = -right.dot(eye);
        m(1, 3) = -up_adjusted.dot(eye);
        m(2, 3) = forward.dot(eye);
        m(3, 3) = T(1);
    }
    return m;
}

// Rotation matrix around an axis
template <NumericT T>
[[nodiscard]] Matrix4x4<T> rotate(const Vector<T, 3> &axis, T angle) noexcept
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
[[nodiscard]] Matrix4x4<T> transpose(const Matrix4x4<T> &m) noexcept
{
    Matrix4x4<T> result{};

    if constexpr (std::is_same_v<T, float> && simdLevel >= SIMDLevel::AVX) {
        // Load 4 columns into AVX registers
        __m128 row0 = _mm_loadu_ps(&m.data[0]);
        __m128 row1 = _mm_loadu_ps(&m.data[4]);
        __m128 row2 = _mm_loadu_ps(&m.data[8]);
        __m128 row3 = _mm_loadu_ps(&m.data[12]);

        // Transpose 4x4 matrix in-place
        _MM_TRANSPOSE4_PS(row0, row1, row2, row3);

        // Store back to result
        _mm_storeu_ps(&result.data[0], row0);
        _mm_storeu_ps(&result.data[4], row1);
        _mm_storeu_ps(&result.data[8], row2);
        _mm_storeu_ps(&result.data[12], row3);
    }
    else if constexpr (std::is_same_v<T, float> && simdLevel >= SIMDLevel::SSE2) {
        // Same as AVX but with SSE
        __m128 row0 = _mm_loadu_ps(&m.data[0]);
        __m128 row1 = _mm_loadu_ps(&m.data[4]);
        __m128 row2 = _mm_loadu_ps(&m.data[8]);
        __m128 row3 = _mm_loadu_ps(&m.data[12]);

        _MM_TRANSPOSE4_PS(row0, row1, row2, row3);

        _mm_storeu_ps(&result.data[0], row0);
        _mm_storeu_ps(&result.data[4], row1);
        _mm_storeu_ps(&result.data[8], row2);
        _mm_storeu_ps(&result.data[12], row3);
    }
    else {
        // Scalar fallback
        for (size_t row = 0; row < 4; ++row) {
            for (size_t col = 0; col < 4; ++col) {
                result(row, col) = m(col, row);
            }
        }
    }
    return result;
}

// Scalar floor
template <typename T>
constexpr int floor(T x)
{
    int i = static_cast<int>(x);
    return x < static_cast<T>(0) && static_cast<T>(i) != x ? i - 1 : i;
}

// SIMD floor for Vec4
inline Vec4 floor(const Vec4 &v)
{
    if (simdLevel >= SIMDLevel::SSE2) {
        __m128 val = _mm_loadu_ps(v.components.data());
        __m128 floored = _mm_floor_ps(val); // SSE4.1
        Vec4 result;
        _mm_storeu_ps(result.components.data(), floored);
        return result;
    }
    else {
        return {std::floor(v.x), std::floor(v.y), std::floor(v.z), std::floor(v.w)};
    }
}

// Floor for Vec3
inline Vec3 floor(const Vec3 &v) { return {std::floor(v.x), std::floor(v.y), std::floor(v.z)}; }

// Floor for Vec2
inline Vec2 floor(const Vec2 &v) { return {std::floor(v.x), std::floor(v.y)}; }

} // namespace math
} // namespace gouda::math