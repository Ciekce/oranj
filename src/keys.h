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

#include <array>

#include "core.h"
#include "util/rng.h"

namespace oranj::keys {
    namespace sizes {
        constexpr usize kPieceSquares = Pieces::kCount * Squares::kCount;
        constexpr usize kColor = 1;

        constexpr auto kTotal = kPieceSquares + kColor;
    } // namespace sizes

    namespace offsets {
        constexpr usize kPieceSquares = 0;
        constexpr auto kColor = kPieceSquares + sizes::kPieceSquares;
    } // namespace offsets

    constexpr auto kKeys = [] {
        constexpr auto kSeed = U64(0xD06C659954EC904A);

        std::array<u64, sizes::kTotal> keys{};

        util::rng::Jsf64Rng rng{kSeed};

        for (auto& key : keys) {
            key = rng.nextU64();
        }

        return keys;
    }();

    inline u64 pieceSquare(Piece piece, Square sq) {
        if (piece == Pieces::kNone || sq == Squares::kNone) {
            return 0;
        }

        return kKeys[offsets::kPieceSquares + sq.idx() * Pieces::kCount + piece.idx()];
    }

    // for flipping
    inline u64 color() {
        return kKeys[offsets::kColor];
    }

    inline u64 color(Color c) {
        return c == Colors::kWhite ? 0 : color();
    }
} // namespace oranj::keys
