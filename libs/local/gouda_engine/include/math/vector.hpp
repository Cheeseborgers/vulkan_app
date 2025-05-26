#pragma once
/**
 * @file math/vector.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-13
 * @brief Engine math module - Generic Vector Implementation
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
#include "math/simd.hpp"

namespace gouda::math {

/**
 * @brief Base class for vector operations with static polymorphism (CRTP).
 *
 * @tparam T Numeric type (e.g., float, int).
 * @tparam N Dimension of the vector.
 * @tparam Derived The derived vector type.
 */
template <NumericT T, size_t N, typename Derived>
class VectorBase {
protected:
    T &getComponent(size_t index) { return static_cast<Derived *>(this)->components[index]; }
    const T &getComponent(size_t index) const { return static_cast<const Derived *>(this)->components[index]; }

public:
    T operator[](size_t index) const
    {
        ASSERT(index < N, "Vector index out of bounds");
        return getComponent(index);
    }

    T &operator[](size_t index)
    {
        ASSERT(index < N, "Vector index out of bounds");
        return getComponent(index);
    }

    Derived operator+(const Derived &other) const
    {
        Derived result;
        if constexpr (N == 4) {
            if (simdLevel >= SIMDLevel::AVX) {
                __m128 a = _mm_loadu_ps(static_cast<const Derived *>(this)->components.data());
                __m128 b = _mm_loadu_ps(other.components.data());
                __m128 r = _mm_add_ps(a, b);
                _mm_storeu_ps(result.components.data(), r);
                return result;
            }
        }
        for (size_t i = 0; i < N; ++i)
            result[i] = (*this)[i] + other[i];
        return result;
    }

    Derived operator-(const Derived &other) const
    {
        Derived result;
        if constexpr (N == 4) {
            if (simdLevel >= SIMDLevel::AVX) {
                __m128 a = _mm_loadu_ps(static_cast<const Derived *>(this)->components.data());
                __m128 b = _mm_loadu_ps(other.components.data());
                __m128 r = _mm_sub_ps(a, b);
                _mm_storeu_ps(result.components.data(), r);
                return result;
            }
        }
        for (size_t i = 0; i < N; ++i)
            result[i] = (*this)[i] - other[i];
        return result;
    }

    Derived operator*(T scalar) const
    {
        Derived result;
        if constexpr (N == 4) {
            if (simdLevel >= SIMDLevel::AVX) {
                __m128 a = _mm_loadu_ps(static_cast<const Derived *>(this)->components.data());
                __m128 s = _mm_set1_ps(scalar);
                __m128 r = _mm_mul_ps(a, s);
                _mm_storeu_ps(result.components.data(), r);
                return result;
            }
        }
        for (size_t i = 0; i < N; ++i)
            result[i] = (*this)[i] * scalar;
        return result;
    }

    Derived operator/(T scalar) const
    {
        ASSERT(scalar != T(0), "Division by zero in Vector");
        Derived result;
        if constexpr (N == 4) {
            if (simdLevel >= SIMDLevel::AVX) {
                __m128 a = _mm_loadu_ps(static_cast<const Derived *>(this)->components.data());
                __m128 s = _mm_set1_ps(scalar);
                __m128 r = _mm_div_ps(a, s);
                _mm_storeu_ps(result.components.data(), r);
                return result;
            }
        }
        for (size_t i = 0; i < N; ++i)
            result[i] = (*this)[i] / scalar;
        return result;
    }

    Derived &operator+=(const Derived &other)
    {
        if constexpr (N == 4) {
            if (simdLevel >= SIMDLevel::AVX) {
                __m128 a = _mm_loadu_ps(static_cast<Derived *>(this)->components.data());
                __m128 b = _mm_loadu_ps(other.components.data());
                a = _mm_add_ps(a, b);
                _mm_storeu_ps(static_cast<Derived *>(this)->components.data(), a);
                return *static_cast<Derived *>(this);
            }
        }
        for (size_t i = 0; i < N; ++i)
            (*this)[i] += other[i];
        return *static_cast<Derived *>(this);
    }

    Derived &operator-=(const Derived &other)
    {
        if constexpr (N == 4) {
            if (simdLevel >= SIMDLevel::AVX) {
                __m128 a = _mm_loadu_ps(static_cast<Derived *>(this)->components.data());
                __m128 b = _mm_loadu_ps(other.components.data());
                a = _mm_sub_ps(a, b);
                _mm_storeu_ps(static_cast<Derived *>(this)->components.data(), a);
                return *static_cast<Derived *>(this);
            }
        }
        for (size_t i = 0; i < N; ++i)
            (*this)[i] -= other[i];
        return *static_cast<Derived *>(this);
    }

    Derived &operator*=(T scalar)
    {
        if constexpr (N == 4) {
            if (simdLevel >= SIMDLevel::AVX) {
                __m128 a = _mm_loadu_ps(static_cast<Derived *>(this)->components.data());
                __m128 s = _mm_set1_ps(scalar);
                a = _mm_mul_ps(a, s);
                _mm_storeu_ps(static_cast<Derived *>(this)->components.data(), a);
                return *static_cast<Derived *>(this);
            }
        }
        for (size_t i = 0; i < N; ++i)
            (*this)[i] *= scalar;
        return *static_cast<Derived *>(this);
    }

    Derived &operator/=(T scalar)
    {
        ASSERT(scalar != T(0), "Division by zero in Vector");
        if constexpr (N == 4) {
            if (simdLevel >= SIMDLevel::AVX) {
                __m128 a = _mm_loadu_ps(static_cast<Derived *>(this)->components.data());
                const __m128 s = _mm_set1_ps(scalar);
                a = _mm_div_ps(a, s);
                _mm_storeu_ps(static_cast<Derived *>(this)->components.data(), a);
                return *static_cast<Derived *>(this);
            }
        }
        for (size_t i = 0; i < N; ++i)
            (*this)[i] /= scalar;
        return *static_cast<Derived *>(this);
    }

    Derived operator-() const
    {
        Derived result;
        if constexpr (N == 4) {
            if (simdLevel >= SIMDLevel::AVX) {
                __m128 a = _mm_loadu_ps(static_cast<const Derived *>(this)->components.data());
                const __m128 zero = _mm_setzero_ps();
                a = _mm_sub_ps(zero, a);
                _mm_storeu_ps(result.components.data(), a);
                return result;
            }
        }
        for (size_t i = 0; i < N; ++i)
            result[i] = -(*this)[i];
        return result;
    }

    bool operator!=(const Derived &other) const { return !(*this == other); }

    bool operator==(const Derived &other) const
    {
        for (size_t i = 0; i < N; ++i)
            if (getComponent(i) != other.getComponent(i))
                return false;
        return true;
    }

    T dot(const Derived &other) const
    {
        T result = T(0);
        if constexpr (N == 4) {
            if (simdLevel >= SIMDLevel::SSE2) {
                const __m128 a = _mm_loadu_ps(static_cast<const Derived *>(this)->components.data());
                const __m128 b = _mm_loadu_ps(other.components.data());
                __m128 mul = _mm_mul_ps(a, b);
                __m128 shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
                __m128 sums = _mm_add_ps(mul, shuf);
                shuf = _mm_movehl_ps(shuf, sums);
                sums = _mm_add_ss(sums, shuf);
                return _mm_cvtss_f32(sums);
            }
        }
        for (size_t i = 0; i < N; ++i)
            result += (*this)[i] * other[i];
        return result;
    }

    [[nodiscard]] f64 magnitude() const
    {
        return std::sqrt(static_cast<f64>(dot(*static_cast<const Derived *>(this))));
    }

    Derived normalized() const
    {
        f64 mag = magnitude();
        return (mag > 0.0) ? (*this / static_cast<T>(mag)) : Derived();
    }

    T squaredDistance(const Derived &other) const
    {
        T sum = T(0);
        for (size_t i = 0; i < N; ++i) {
            T diff = (*this)[i] - other[i];
            sum += diff * diff;
        }
        return sum;
    }

    f64 distance(const Derived &other) const { return std::sqrt(static_cast<f64>(squaredDistance(other))); }
};

// Primary templated Vector class for N dimensions
template <NumericT T, size_t N>
class Vector : public VectorBase<T, N, Vector<T, N>> {
public:
    std::array<T, N> components;

    // Constructors
    Vector() { components.fill(T(0)); }

    template <typename... Args>
    explicit Vector(Args... args) : components{{static_cast<T>(args)...}}
    {
        ASSERT(sizeof...(args) == N, "Number of arguments must match vector dimension");
    }

    // Cross product (only for 3D vectors)
    Vector cross(const Vector &other) const
        requires(N == 3)
    {
        return Vector(components[1] * other[2] - components[2] * other[1], // x
                      components[2] * other[0] - components[0] * other[2], // y
                      components[0] * other[1] - components[1] * other[0]  // z
        );
    }
};

// Specialization for Vec2 (N=2)
template <NumericT T>
class Vector<T, 2> : public VectorBase<T, 2, Vector<T, 2>> {
public:
    union {
        std::array<T, 2> components;
        struct {
            T x, y;
        };
    };

    // Constructors
    Vector() { components.fill(T(0)); }
    Vector(T value) { components.fill(T(value)); }
    Vector(T x_, T y_) : components{x_, y_} {}

    [[nodiscard]] String ToString() const { return std::format("x:{}, y:{}", x, y); }
};

// Specialization for Vec3 (N=3)
template <NumericT T>
class Vector<T, 3> : public VectorBase<T, 3, Vector<T, 3>> {
public:
    union {
        std::array<T, 3> components;
        struct {
            T x, y, z;
        };
    };

    // Constructors
    Vector() { components.fill(T(0)); }
    Vector(T value) { components.fill(T(value)); }
    Vector(T x_, T y_, T z_) : components{x_, y_, z_} {}

    // Cross product (specific to 3D)
    Vector cross(const Vector &other) const
    {
        return Vector(y * other.z - z * other.y, // x
                      z * other.x - x * other.z, // y
                      x * other.y - y * other.x  // z
        );
    }

    [[nodiscard]] String ToString() const { return std::format("x:{}, y:{}, z:{}", x, y, z); }
};

// Specialization for Vec4 (N=4)
template <NumericT T>
class Vector<T, 4> : public VectorBase<T, 4, Vector<T, 4>> {
public:
    union {
        std::array<T, 4> components;
        struct {
            T x, y, z, w;
        };
    };

    // Constructors
    Vector() { components.fill(T(0)); }
    Vector(T value) { components.fill(T(value)); }
    Vector(T x_, T y_, T z_, T w_) : components{x_, y_, z_, w_} {}

    [[nodiscard]] String ToString() const { return std::format("x:{}, y:{}, z:{}, w:{}", x, y, z, w); }
};

// Scalar multiplication (left-hand scalar)
template <NumericT T, size_t N>
Vector<T, N> operator*(T scalar, const Vector<T, N> &vec)
{
    return vec * scalar;
}

// Type aliases
using Vec2 = Vector<float, 2>;
using Vec3 = Vector<float, 3>;
using Vec4 = Vector<float, 4>;

using UVec2 = Vector<u_int32_t, 2>;
using UVec3 = Vector<u_int32_t, 3>;
using UVec4 = Vector<u_int32_t, 4>;

}