/*
 * oranj, a UCI shatranj engine
 * Copyright (C) 2025 Ciekce
 *
 * oranj is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * oranj is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with oranj. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "types.h"

#include <new>

#if defined(OJ_NATIVE)
    // cannot expand a macro to defined()
    #if __BMI2__ && defined(OJ_FAST_PEXT)
        #define OJ_HAS_BMI2 1
    #else
        #define OJ_HAS_BMI2 0
    #endif
    #if !defined(OJ_DISABLE_AVX512)
        #define OJ_HAS_VNNI512 __AVX512VNNI__
        #define OJ_HAS_AVX512 (__AVX512F__ && (__AVX512BW__ || __AVX512VNNI__))
    #else
        #define OJ_HAS_VNNI512 0
        #define OJ_HAS_AVX512 0
    #endif
    #define OJ_HAS_VNNI256 0 // slowdown on any cpu that would use it
    #define OJ_HAS_AVX2 __AVX2__
    #define OJ_HAS_POPCNT __POPCNT__
    #define OJ_HAS_NEON __ARM_NEON
    #if !defined(OJ_DISABLE_NEON_DOTPROD)
        #define OJ_HAS_NEON_DOTPROD (__ARM_ARCH >= 8)
    #else
        #define OJ_HAS_NEON_DOTPROD 0
    #endif
#elif defined(OJ_VNNI512)
    #define OJ_HAS_BMI2 1
    #define OJ_HAS_VNNI512 1
    #define OJ_HAS_AVX512 1
    #define OJ_HAS_VNNI256 1
    #define OJ_HAS_AVX2 1
    #define OJ_HAS_POPCNT 1
    #define OJ_HAS_NEON 0
    #define OJ_HAS_NEON_DOTPROD 0
#elif defined(OJ_AVX512)
    #define OJ_HAS_BMI2 1
    #define OJ_HAS_VNNI512 0
    #define OJ_HAS_AVX512 1
    #define OJ_HAS_VNNI256 0
    #define OJ_HAS_AVX2 1
    #define OJ_HAS_POPCNT 1
    #define OJ_HAS_NEON 0
    #define OJ_HAS_NEON_DOTPROD 0
#elif defined(OJ_AVX2_BMI2)
    #define OJ_HAS_BMI2 1
    #define OJ_HAS_VNNI512 0
    #define OJ_HAS_AVX512 0
    #define OJ_HAS_VNNI256 0
    #define OJ_HAS_AVX2 1
    #define OJ_HAS_POPCNT 1
    #define OJ_HAS_NEON 0
    #define OJ_HAS_NEON_DOTPROD 0
#elif defined(OJ_AVX2)
    #define OJ_HAS_BMI2 0
    #define OJ_HAS_VNNI512 0
    #define OJ_HAS_AVX512 0
    #define OJ_HAS_VNNI256 0
    #define OJ_HAS_AVX2 1
    #define OJ_HAS_POPCNT 1
    #define OJ_HAS_NEON 0
    #define OJ_HAS_NEON_DOTPROD 0
#else
    #error no arch specified
#endif

namespace oranj {
#ifdef __cpp_lib_hardware_interference_size
    constexpr auto kCacheLineSize = std::hardware_destructive_interference_size;
#else
    constexpr usize kCacheLineSize = 64;
#endif
} // namespace oranj
