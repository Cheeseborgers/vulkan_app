#pragma once
/**
 * @file math/vector.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-13
 * @brief Engine math module - Generic Vector Implementation with SIMD
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

#include "core/types.hpp"
#include "debug/assert.hpp"
#include <array>
#include <cmath>

#include <immintrin.h>

#if defined(_MSC_VER) // Microsoft Compiler (MSVC)
#include <intrin.h>
#else // GCC / Clang
#include <cpuid.h>
#endif

namespace gouda {
namespace math {

// SIMD support detection
enum class SIMDLevel { NONE, SSE2, AVX, AVX2 };
inline SIMDLevel DetectSIMD()
{
    int cpuInfo[4] = {0};

#if defined(_MSC_VER) // MSVC Version
    __cpuid(cpuInfo, 0);
    if (cpuInfo[0] >= 7) {
        __cpuidex(cpuInfo, 7, 0);
        if (cpuInfo[1] & (1 << 5))
            return SIMDLevel::AVX2;
    }
    __cpuid(cpuInfo, 1);
    if (cpuInfo[2] & (1 << 28))
        return SIMDLevel::AVX;
    if (cpuInfo[3] & (1 << 26))
        return SIMDLevel::SSE2;

#elif defined(__GNUC__) || defined(__clang__) // GCC / Clang
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(0, &eax, &ebx, &ecx, &edx)) {
        if (eax >= 7) {
            __get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx);
            if (ebx & (1 << 5))
                return SIMDLevel::AVX2;
        }
        __get_cpuid(1, &eax, &ebx, &ecx, &edx);
        if (ecx & (1 << 28))
            return SIMDLevel::AVX;
        if (edx & (1 << 26))
            return SIMDLevel::SSE2;
    }
#endif

    return SIMDLevel::NONE;
}
static const SIMDLevel simdLevel = DetectSIMD();

// Base vector class
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
                __m128 a = _mm_loadu_ps(this->components.data());
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

    T Dot(const Derived &other) const
    {
        T result = T(0);
        if constexpr (N == 4) {
            if (simdLevel >= SIMDLevel::SSE2) {
                __m128 a = _mm_loadu_ps(this->components.data());
                __m128 b = _mm_loadu_ps(other.components.data());
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
};

// Vector class
template <NumericT T, size_t N>
class Vector : public VectorBase<T, N, Vector<T, N>> {
public:
    std::array<T, N> components;
    Vector() { components.fill(T(0)); }
};

using Vec2 = Vector<float, 2>;
using Vec3 = Vector<float, 3>;
using Vec4 = Vector<float, 4>;

} // namespace math
} // namespace gouda
