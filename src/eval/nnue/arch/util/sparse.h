/*
 * oranj, a UCI shatranj engine
 * Copyright (C) 2026 Ciekce
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

#include "../../../../types.h"

#include <array>

#include "../../../../arch.h"

#define OJ_SPARSE_BENCH_L1_SIZE 0

#if OJ_HAS_AVX512 && OJ_HAS_VBMI2
    #include "sparse_vbmi2.h"
#else
    #include "sparse_default.h"
#endif

namespace oranj::eval::nnue::arch::sparse {
#if OJ_SPARSE_BENCH_L1_SIZE > 0
    inline std::array<usize, OJ_SPARSE_BENCH_L1_SIZE / 2> g_activationCounts{};
    inline void trackActivations(std::span<const u8, OJ_SPARSE_BENCH_L1_SIZE> ftActivations) {
        for (usize i = 0; i < OJ_SPARSE_BENCH_L1_SIZE; ++i) {
            if (ftActivations[i] != 0) {
                ++g_activationCounts[i % (OJ_SPARSE_BENCH_L1_SIZE / 2)];
            }
        }
    }
#endif
} // namespace oranj::eval::nnue::arch::sparse
