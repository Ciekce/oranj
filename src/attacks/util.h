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

#include "../types.h"

#include <array>

#include "../bitboard.h"
#include "../core.h"
#include "../util/cemath.h"

namespace oranj::attacks {
    namespace internal {
        constexpr Bitboard edges(i32 dir) {
            switch (dir) {
                case offsets::kUp:
                    return boards::kRank8;
                case offsets::kDown:
                    return boards::kRank1;
                case offsets::kLeft:
                    return boards::kFileA;
                case offsets::kRight:
                    return boards::kFileH;
                default:
                    __builtin_unreachable(); // don't
            }
        }

        constexpr Bitboard generateSlidingAttacks(Square src, i32 dir, Bitboard occupancy) {
            Bitboard dst{};

            auto blockers = edges(dir);

            const bool right = dir < 0;
            const auto shift = util::abs(dir);

            auto bit = squareBit(src);

            if (!(blockers & bit).empty()) {
                return dst;
            }

            blockers |= occupancy;

            do {
                if (right) {
                    dst |= bit >>= shift;
                } else {
                    dst |= bit <<= shift;
                }
            } while (!(bit & blockers));

            return dst;
        }
    } // namespace internal

    constexpr auto kEmptyBoardRooks = [] {
        std::array<Bitboard, 64> dst{};

        for (i32 square = 0; square < 64; ++square) {
            for (const auto dir : {offsets::kUp, offsets::kDown, offsets::kLeft, offsets::kRight}) {
                const auto attacks = internal::generateSlidingAttacks(static_cast<Square>(square), dir, 0);
                dst[square] |= attacks;
            }
        }

        return dst;
    }();

    consteval Bitboard genRookAttacks(Square src, Bitboard occupancy) {
        Bitboard dst{};

        for (const auto dir : {offsets::kUp, offsets::kDown, offsets::kLeft, offsets::kRight}) {
            dst |= internal::generateSlidingAttacks(src, dir, occupancy);
        }

        return dst;
    }
} // namespace oranj::attacks
