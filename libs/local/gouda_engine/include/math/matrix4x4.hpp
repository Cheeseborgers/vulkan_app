#pragma once
/**
 * @file math/matrix4x4.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-10
 * @brief 4x4 Matrix for engine transformations
 *
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <array>
#include <cmath>
#include <immintrin.h>

#include "core/types.hpp"
#include "debug/assert.hpp"
#include "math/vector.hpp"

namespace gouda::math {

template <typename T>
class Matrix4x4 {
    static_assert(std::is_same_v<T, float>, "Matrix4x4 only supports float for SIMD compatibility");

public:
    alignas(16) std::array<T, 16> data; // Column-major storage

    // Default constructor: Identity matrix
    Matrix4x4() {
        data = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    }

    // Constructor with explicit values
    explicit Matrix4x4(const std::array<T, 16> &values) : data(values) {}

    static Matrix4x4 identity() { return Matrix4x4(); }

    // Access element (row, col)
    T operator()(const size_t row, const size_t col) const {
        ASSERT(row < 4 && col < 4, "Matrix4x4 index out of bounds");
        return data[row + col * 4]; // Column-major: (row, col) = data[row + col*4]
    }

    T &operator()(const size_t row, const size_t col) {
        ASSERT(row < 4 && col < 4, "Matrix4x4 index out of bounds");
        return data[row + col * 4];
    }

    // Matrix multiplication (SIMD optimized)
    Matrix4x4 operator*(const Matrix4x4 &other) const {
        Matrix4x4 result;

#if defined(__SSE4_1___)
        // SSE4.1: Use _mm_dp_ps for dot products
        for (size_t col = 0; col < 4; ++col) {
            __m128 bcol = _mm_load_ps(&other.data[col * 4]); // Load column of other
            for (size_t row = 0; row < 4; row += 4) {
                // Load 4 rows of this matrix
                __m128 arow0 = _mm_load_ps(&data[row * 4]);
                __m128 arow1 = _mm_load_ps(&data[(row + 1) * 4]);
                __m128 arow2 = _mm_load_ps(&data[(row + 2) * 4]);
                __m128 arow3 = _mm_load_ps(&data[(row + 3) * 4]);

                // Compute dot products
                result.data[row * 4 + col] = _mm_cvtss_f32(_mm_dp_ps(arow0, bcol, 0xF1));
                result.data[(row + 1) * 4 + col] = _mm_cvtss_f32(_mm_dp_ps(arow1, bcol, 0xF1));
                result.data[(row + 2) * 4 + col] = _mm_cvtss_f32(_mm_dp_ps(arow2, bcol, 0xF1));
                result.data[(row + 3) * 4 + col] = _mm_cvtss_f32(_mm_dp_ps(arow3, bcol, 0xF1));
            }
        }
#elif defined(__SSE__)
        // SSE: Manual summation
        for (size_t col = 0; col < 4; ++col) {
            __m128 bcol = _mm_load_ps(&other.data[col * 4]);
            for (size_t row = 0; row < 4; ++row) {
                // Load row of this matrix (transpose for correct access)
                __m128 arow = _mm_load_ps(&data[row * 4]);
                __m128 prod = _mm_mul_ps(arow, bcol);
                // Sum: [x,y,z,w] -> x+y+z+w
                __m128 shuf = _mm_shuffle_ps(prod, prod, _MM_SHUFFLE(2, 3, 0, 1));
                __m128 sums = _mm_add_ps(prod, shuf);
                __m128 shuf2 = _mm_shuffle_ps(sums, sums, _MM_SHUFFLE(0, 1, 2, 3));
                __m128 final = _mm_add_ps(sums, shuf2);
                result.data[row + col * 4] = _mm_cvtss_f32(final);
            }
        }
#else
        // Scalar: Unrolled for performance
        for (size_t row = 0; row < 4; ++row) {
            for (size_t col = 0; col < 4; ++col) {
                T sum = T(0);
                sum += data[row + 0 * 4] * other.data[0 + col * 4];
                sum += data[row + 1 * 4] * other.data[1 + col * 4];
                sum += data[row + 2 * 4] * other.data[2 + col * 4];
                sum += data[row + 3 * 4] * other.data[3 + col * 4];
                result.data[row + col * 4] = sum;
            }
        }
#endif
        return result;
    }

    // Transform a Vector<T, 4> (SIMD optimized)
    Vector<T, 4> operator*(const Vector<T, 4> &vec) const {
        Vector<T, 4> result;

#if defined(__SSE4_1__)
        __m128 v = _mm_loadu_ps(vec.data()); // Unaligned load for vector
        for (size_t row = 0; row < 4; ++row) {
            __m128 mrow = _mm_load_ps(&data[row * 4]); // Load matrix row
            result[row] = _mm_cvtss_f32(_mm_dp_ps(mrow, v, 0xF1));
        }
#elif defined(__SSE__)
        __m128 v = _mm_loadu_ps(vec.data());
        for (size_t row = 0; row < 4; ++row) {
            __m128 mrow = _mm_load_ps(&data[row * 4]);
            __m128 prod = _mm_mul_ps(mrow, v);
            __m128 shuf = _mm_shuffle_ps(prod, prod, _MM_SHUFFLE(2, 3, 0, 1));
            __m128 sums = _mm_add_ps(prod, shuf);
            __m128 shuf2 = _mm_shuffle_ps(sums, sums, _MM_SHUFFLE(0, 1, 2, 3));
            __m128 final = _mm_add_ps(sums, shuf2);
            result[row] = _mm_cvtss_f32(final);
        }
#else
        for (size_t row = 0; row < 4; ++row) {
            T sum = T(0);
            sum += data[row + 0 * 4] * vec[0];
            sum += data[row + 1 * 4] * vec[1];
            sum += data[row + 2 * 4] * vec[2];
            sum += data[row + 3 * 4] * vec[3];
            result[row] = sum;
        }
#endif
        return result;
    }

    // Common transformation matrices
    static Matrix4x4 translation(const Vector<T, 3> &t) {
        Matrix4x4 m;
        m.data[12] = t[0]; // (0,3)
        m.data[13] = t[1]; // (1,3)
        m.data[14] = t[2]; // (2,3)
        return m;
    }

    static Matrix4x4 rotationX(T angle) {
        T c = std::cos(angle);
        T s = std::sin(angle);
        Matrix4x4 m;
        m.data[5] = c;  // (1,1)
        m.data[6] = -s; // (2,1)
        m.data[9] = s;  // (1,2)
        m.data[10] = c; // (2,2)
        return m;
    }

    static Matrix4x4 rotationY(T angle) {
        T c = std::cos(angle);
        T s = std::sin(angle);
        Matrix4x4 m;
        m.data[0] = c;   // (0,0)
        m.data[2] = s;   // (2,0)
        m.data[8] = -s;  // (0,2)
        m.data[10] = c;  // (2,2)
        return m;
    }

    static Matrix4x4 rotationZ(T angle) {
        T c = std::cos(angle);
        T s = std::sin(angle);
        Matrix4x4 m;
        m.data[0] = c;  // (0,0)
        m.data[1] = -s; // (1,0)
        m.data[4] = s;  // (0,1)
        m.data[5] = c;  // (1,1)
        return m;
    }

    static Matrix4x4 scaling(const Vector<T, 3> &s) {
        Matrix4x4 m;
        m.data[0] = s[0];  // (0,0)
        m.data[5] = s[1];  // (1,1)
        m.data[10] = s[2]; // (2,2)
        return m;
    }

    // Orthographic projection
    static Matrix4x4 ortho(T left, T right, T bottom, T top, T near, T far) {
        Matrix4x4 m;
        m.data[0] = T(2) / (right - left);               // (0,0)
        m.data[5] = T(2) / (top - bottom);               // (1,1)
        m.data[10] = T(2) / (near - far);                // (2,2)
        m.data[12] = -(right + left) / (right - left);   // (0,3)
        m.data[13] = -(top + bottom) / (top - bottom);   // (1,3)
        m.data[14] = -(far + near) / (far - near);       // (2,3)
        m.data[15] = T(1);                               // (3,3)
        return m;
    }

    const T *getData() const { return data.data(); } // For graphics APIs
};

using Mat4 = Matrix4x4<float>;

} // namespace gouda::math
