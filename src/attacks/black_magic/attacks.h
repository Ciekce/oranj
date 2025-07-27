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

#include "../../types.h"

#include <array>

#include "../../bitboard.h"
#include "../../core.h"
#include "../../util/bits.h"
#include "../util.h"
#include "data.h"

namespace oranj::attacks {
    extern const std::array<Bitboard, black_magic::kRookData.tableSize> g_rookAttacks;

    [[nodiscard]] inline usize getRookIdx(Bitboard occupancy, Square src) {
        const auto s = static_cast<i32>(src);

        const auto& data = black_magic::kRookData.data[s];

        const auto magic = black_magic::kMagics[s];
        const auto shift = black_magic::kShifts[s];

        return ((occupancy | data.mask) * magic) >> shift;
    }

    [[nodiscard]] inline Bitboard getRookAttacks(Square src, Bitboard occupancy) {
        const auto s = static_cast<i32>(src);

        const auto& data = black_magic::kRookData.data[s];
        const auto idx = getRookIdx(occupancy, src);

        return g_rookAttacks[data.offset + idx];
    }
} // namespace oranj::attacks
