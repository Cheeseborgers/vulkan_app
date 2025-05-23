#pragma once
/**
 * @file math/mat4x4.hpp
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
#include <array>
#include <cmath>
#include <immintrin.h>

#include "core/types.hpp"
#include "debug/assert.hpp"
#include "math/simd.hpp"
#include "math/vector.hpp"

namespace gouda::math {

template <NumericT T>
class Matrix4x4 {
public:
    std::array<T, 16> data; // Column-major storage

    // Default constructor: Identity matrix
    Matrix4x4() { data = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; }

    // Constructor with explicit values
    explicit Matrix4x4(const std::array<T, 16> &values) : data(values) {}

    static Matrix4x4 identity() { return Matrix4x4(); }

    // Access element (row, col)
    T operator()(const size_t row, const size_t col) const
    {
        ASSERT(row < 4 && col < 4, "Matrix4x4 index out of bounds");
        return data[col * 4 + row];
    }

    T &operator()(const size_t row, const size_t col)
    {
        ASSERT(row < 4 && col < 4, "Matrix4x4 index out of bounds");
        return data[col * 4 + row];
    }

    // Matrix multiplication (SIMD optimized)
    Matrix4x4 operator*(const Matrix4x4 &other) const
    {
        Matrix4x4 result;

// TODO: Use sse enum classes
#if defined(__SSE__)
        for (size_t col = 0; col < 4; ++col) {
            const __m128 bcol = _mm_load_ps(&other.data[col * 4]);
            for (size_t row = 0; row < 4; ++row) {
                const __m128 arow = _mm_set_ps(data[row + 12], data[row + 8], data[row + 4], data[row]);
                const __m128 prod = _mm_mul_ps(arow, bcol);
                result(row, col) = _mm_cvtss_f32(_mm_hadd_ps(_mm_hadd_ps(prod, prod), prod));
            }
        }
#else
        // Scalar fallback
        for (size_t row = 0; row < 4; ++row) {
            for (size_t col = 0; col < 4; ++col) {
                T sum = T(0);
                for (size_t k = 0; k < 4; ++k) {
                    sum += (*this)(row, k) * other(k, col);
                }
                result(row, col) = sum;
            }
        }
#endif
        return result;
    }

    // Transform a Vector<T, 4> (SIMD optimized)
    Vector<T, 4> operator*(const Vector<T, 4> &vec) const
    {
        Vector<T, 4> result;

#if defined(__SSE__)
        __m128 v = _mm_load_ps(vec.data());
        for (size_t row = 0; row < 4; ++row) {
            const __m128 mrow = _mm_set_ps(data[row + 12], data[row + 8], data[row + 4], data[row]);
            const __m128 prod = _mm_mul_ps(mrow, v);
            result[row] = _mm_cvtss_f32(_mm_hadd_ps(_mm_hadd_ps(prod, prod), prod));
        }
#else
        for (size_t row = 0; row < 4; ++row) {
            T sum = T(0);
            for (size_t col = 0; col < 4; ++col) {
                sum += (*this)(row, col) * vec[col];
            }
            result[row] = sum;
        }
#endif
        return result;
    }

    // Common transformation matrices
    static Matrix4x4 translation(const Vector<T, 3> &t)
    {
        Matrix4x4 m;
        m(0, 3) = t[0];
        m(1, 3) = t[1];
        m(2, 3) = t[2];
        return m;
    }

    static Matrix4x4 rotationX(T angle)
    {
        T c = std::cos(angle);
        T s = std::sin(angle);
        Matrix4x4 m;
        m(1, 1) = c;
        m(1, 2) = -s;
        m(2, 1) = s;
        m(2, 2) = c;
        return m;
    }

    static Matrix4x4 rotationY(T angle)
    {
        T c = std::cos(angle);
        T s = std::sin(angle);
        Matrix4x4 m;
        m(0, 0) = c;
        m(0, 2) = s;
        m(2, 0) = -s;
        m(2, 2) = c;
        return m;
    }

    static Matrix4x4 rotationZ(T angle)
    {
        T c = std::cos(angle);
        T s = std::sin(angle);
        Matrix4x4 m;
        m(0, 0) = c;
        m(0, 1) = -s;
        m(1, 0) = s;
        m(1, 1) = c;
        return m;
    }

    static Matrix4x4 Scaling(const Vector<T, 3> &s)
    {
        Matrix4x4 m;
        m(0, 0) = s[0];
        m(1, 1) = s[1];
        m(2, 2) = s[2];
        return m;
    }

    const T *getData() const { return data.data(); } // For passing to graphics APIs
};

} // namespace gouda::math
