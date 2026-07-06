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

#include "types.h"

#include "search.h"

namespace oranj::bench {
#if OJ_SPARSE_BENCH_L1_SIZE > 0
    constexpr i32 kDefaultBenchDepth = 22;
#else
    constexpr i32 kDefaultBenchDepth = 19;
#endif

    constexpr usize kDefaultBenchTtSize = 16;

    void run(i32 depth = kDefaultBenchDepth, usize ttSize = kDefaultBenchTtSize);
} // namespace oranj::bench
