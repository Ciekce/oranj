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

#include "../types.h"

#include <array>
#include <cassert>

#include "../bitboard.h"
#include "../core.h"
#include "../util/bits.h"
#include "util.h"

#if OJ_HAS_BMI2
    #include "bmi2/attacks.h"
#else
    #include "black_magic/attacks.h"
#endif

namespace oranj::attacks {
    consteval std::array<Bitboard, Squares::kCount> generatePawnAttacks(Color us) {
        std::array<Bitboard, Squares::kCount> dst{};

        for (usize i = 0; i < dst.size(); ++i) {
            const auto bit = Bitboard::fromSquare(Square::fromRaw(i));

            dst[i] |= bit.shiftUpLeftRelative(us);
            dst[i] |= bit.shiftUpRightRelative(us);
        }

        return dst;
    }

    constexpr auto kBlackPawnAttacks = generatePawnAttacks(Colors::kBlack);
    constexpr auto kWhitePawnAttacks = generatePawnAttacks(Colors::kWhite);

    constexpr auto kAlfilAttacks = [] {
        std::array<Bitboard, Squares::kCount> dst{};

        for (usize i = 0; i < dst.size(); ++i) {
            const auto bit = Bitboard::fromSquare(Square::fromRaw(i));

            auto& attacks = dst[i];

            attacks |= bit.shiftUpLeft().shiftUpLeft();
            attacks |= bit.shiftUpRight().shiftUpRight();
            attacks |= bit.shiftDownLeft().shiftDownLeft();
            attacks |= bit.shiftDownRight().shiftDownRight();
        }

        return dst;
    }();

    constexpr auto kFerzAttacks = [] {
        std::array<Bitboard, Squares::kCount> dst{};

        for (usize i = 0; i < dst.size(); ++i) {
            const auto bit = Bitboard::fromSquare(Square::fromRaw(i));

            auto& attacks = dst[i];

            attacks |= bit.shiftUpLeft();
            attacks |= bit.shiftUpRight();
            attacks |= bit.shiftDownLeft();
            attacks |= bit.shiftDownRight();
        }

        return dst;
    }();

    constexpr auto kKnightAttacks = [] {
        std::array<Bitboard, Squares::kCount> dst{};

        for (usize i = 0; i < dst.size(); ++i) {
            const auto bit = Bitboard::fromSquare(Square::fromRaw(i));

            auto& attacks = dst[i];

            attacks |= bit.shiftUpUpLeft();
            attacks |= bit.shiftUpUpRight();
            attacks |= bit.shiftUpLeftLeft();
            attacks |= bit.shiftUpRightRight();
            attacks |= bit.shiftDownLeftLeft();
            attacks |= bit.shiftDownRightRight();
            attacks |= bit.shiftDownDownLeft();
            attacks |= bit.shiftDownDownRight();
        }

        return dst;
    }();

    constexpr auto kKingAttacks = [] {
        std::array<Bitboard, Squares::kCount> dst{};

        for (usize i = 0; i < dst.size(); ++i) {
            const auto bit = Bitboard::fromSquare(Square::fromRaw(i));

            auto& attacks = dst[i];

            attacks |= bit.shiftUp();
            attacks |= bit.shiftDown();
            attacks |= bit.shiftLeft();
            attacks |= bit.shiftRight();
            attacks |= bit.shiftUpLeft();
            attacks |= bit.shiftUpRight();
            attacks |= bit.shiftDownLeft();
            attacks |= bit.shiftDownRight();
        }

        return dst;
    }();

    [[nodiscard]] constexpr Bitboard getPawnAttacks(Square src, Color color) {
        if (color == Colors::kBlack) {
            return kBlackPawnAttacks[src.idx()];
        } else {
            return kWhitePawnAttacks[src.idx()];
        }
    }

    [[nodiscard]] constexpr Bitboard getAlfilAttacks(Square src) {
        return kAlfilAttacks[src.idx()];
    }

    [[nodiscard]] constexpr Bitboard getFerzAttacks(Square src) {
        return kFerzAttacks[src.idx()];
    }

    [[nodiscard]] constexpr Bitboard getKnightAttacks(Square src) {
        return kKnightAttacks[src.idx()];
    }

    [[nodiscard]] constexpr Bitboard getKingAttacks(Square src) {
        return kKingAttacks[src.idx()];
    }

    [[nodiscard]] constexpr Bitboard getRookAttacks(Square src, Bitboard occ) {
        if (std::is_constant_evaluated()) {
            return occ.empty() ? kEmptyBoardRooks[src.idx()] : genRookAttacks(src, occ);
        }
        return lookup::getRookAttacks(src, occ);
    }

    [[nodiscard]] constexpr Bitboard getAttacks(Piece piece, Square src, Bitboard occ = Bitboard{}) {
        assert(piece != Pieces::kNone);

        switch (piece.type().raw()) {
            case PieceTypes::kPawn.raw():
                return getPawnAttacks(src, piece.color());
            case PieceTypes::kAlfil.raw():
                return getAlfilAttacks(src);
            case PieceTypes::kFerz.raw():
                return getFerzAttacks(src);
            case PieceTypes::kKnight.raw():
                return getKnightAttacks(src);
            case PieceTypes::kRook.raw():
                return getRookAttacks(src, occ);
            case PieceTypes::kKing.raw():
                return getKingAttacks(src);
            default:
                return Bitboard{};
        }
    }

    [[nodiscard]] constexpr Bitboard getPseudoAttacks(Piece piece, Square src) {
        assert(piece != Pieces::kNone);

        switch (piece.type().raw()) {
            case PieceTypes::kPawn.raw():
                return getPawnAttacks(src, piece.color());
            case PieceTypes::kAlfil.raw():
                return getAlfilAttacks(src);
            case PieceTypes::kFerz.raw():
                return getFerzAttacks(src);
            case PieceTypes::kKnight.raw():
                return getKnightAttacks(src);
            case PieceTypes::kRook.raw():
                return getRookAttacks(src, Bitboard{});
            case PieceTypes::kKing.raw():
                return getKingAttacks(src);
            default:
                return Bitboard{};
        }
    }
} // namespace oranj::attacks
