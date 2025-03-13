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

#include "core/types.hpp"
#include "debug/assert.hpp"
#include "math/vector.hpp"

namespace gouda::math {

template <NumericT T>
class Matrix4x4 {
public:
    std::array<T, 16> data; // Column-major: data[0-3] = column 0, data[4-7] = column 1, etc.

    // Default constructor: Identity matrix
    Matrix4x4() { data = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; }

    // Constructor with explicit values (column-major order)
    Matrix4x4(const std::array<T, 16> &values) : data(values) {}

    // Access element (row, col)
    T operator()(size_t row, size_t col) const
    {
        ASSERT(row < 4 && col < 4, "Matrix4x4 index out of bounds");
        return data[col * 4 + row];
    }

    T &operator()(size_t row, size_t col)
    {
        ASSERT(row < 4 && col < 4, "Matrix4x4 index out of bounds");
        return data[col * 4 + row];
    }

    // Matrix multiplication
    Matrix4x4 operator*(const Matrix4x4 &other) const
    {
        Matrix4x4 result;
        for (size_t row = 0; row < 4; ++row) {
            for (size_t col = 0; col < 4; ++col) {
                T sum = T(0);
                for (size_t k = 0; k < 4; ++k) {
                    sum += (*this)(row, k) * other(k, col);
                }
                result(row, col) = sum;
            }
        }
        return result;
    }

    // Transform a Vector<T, 4>
    Vector<T, 4> operator*(const Vector<T, 4> &vec) const
    {
        Vector<T, 4> result;
        for (size_t row = 0; row < 4; ++row) {
            T sum = T(0);
            for (size_t col = 0; col < 4; ++col) {
                sum += (*this)(row, col) * vec[col];
            }
            result[row] = sum;
        }
        return result;
    }

    // Common transformation matrices
    static Matrix4x4 Translation(const Vector<T, 3> &t)
    {
        Matrix4x4 m;
        m(0, 3) = t[0];
        m(1, 3) = t[1];
        m(2, 3) = t[2];
        return m;
    }

    static Matrix4x4 RotationX(T angle)
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

    static Matrix4x4 Scaling(const Vector<T, 3> &s)
    {
        Matrix4x4 m;
        m(0, 0) = s[0];
        m(1, 1) = s[1];
        m(2, 2) = s[2];
        return m;
    }

    const T *Data() const { return data.data(); } // For passing to graphics APIs
};

} // namespace gouda::math