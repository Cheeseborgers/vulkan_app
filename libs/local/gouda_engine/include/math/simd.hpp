#pragma once

/**
 * @file math/simd.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-14
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
#include <immintrin.h>

#if defined(_MSC_VER)
#include <intrin.h>
#else // GCC / Clang
#include <cpuid.h>
#endif

namespace gouda {
namespace math {

// Enumeration to define available SIMD levels (NONE, SSE2, AVX, AVX2)
enum class SIMDLevel : uint8_t { NONE, SSE2, AVX, AVX2 };

// Function to detect available SIMD capabilities of the CPU
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

// Detect and store the SIMD level at compile-time
static const SIMDLevel simdLevel = DetectSIMD();

} // namespace math
} // namespace gouda