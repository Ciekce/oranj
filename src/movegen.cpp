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

#include "movegen.h"

#include <algorithm>
#include <array>

#include "attacks/attacks.h"
#include "opts.h"
#include "rays.h"
#include "util/bitfield.h"

namespace oranj {
    namespace {
        inline void pushStandards(ScoredMoveList& dst, i32 offset, Bitboard board) {
            while (!board.empty()) {
                const auto dstSquare = board.popLowestSquare();
                const auto srcSquare = static_cast<Square>(static_cast<i32>(dstSquare) - offset);

                dst.push({Move::standard(srcSquare, dstSquare), 0});
            }
        }

        inline void pushStandards(ScoredMoveList& dst, Square srcSquare, Bitboard board) {
            while (!board.empty()) {
                const auto dstSquare = board.popLowestSquare();
                dst.push({Move::standard(srcSquare, dstSquare), 0});
            }
        }

        inline void pushPromotions(ScoredMoveList& noisy, i32 offset, Bitboard board) {
            while (!board.empty()) {
                const auto dstSquare = board.popLowestSquare();
                const auto srcSquare = static_cast<Square>(static_cast<i32>(dstSquare) - offset);

                noisy.push({Move::promotion(srcSquare, dstSquare), 0});
            }
        }

        template <Color kUs>
        void generatePawnsNoisy_(ScoredMoveList& noisy, const Position& pos, Bitboard dstMask) {
            static constexpr auto kThem = oppColor(kUs);

            static constexpr auto kPromotionRank = boards::promotionRank<kUs>();

            static constexpr auto kForwardOffset = offsets::up<kUs>();
            static constexpr auto kLeftOffset = offsets::upLeft<kUs>();
            static constexpr auto kRightOffset = offsets::upRight<kUs>();

            const auto& bbs = pos.bbs();

            const auto theirs = bbs.occupancy<kThem>();

            const auto forwardDstMask = dstMask & kPromotionRank & ~theirs;

            const auto pawns = bbs.pawns<kUs>();

            const auto leftAttacks = pawns.template shiftUpLeftRelative<kUs>() & dstMask;
            const auto rightAttacks = pawns.template shiftUpRightRelative<kUs>() & dstMask;

            pushPromotions(noisy, kLeftOffset, leftAttacks & theirs & kPromotionRank);
            pushPromotions(noisy, kRightOffset, rightAttacks & theirs & kPromotionRank);

            const auto forwards = pawns.template shiftUpRelative<kUs>() & forwardDstMask;
            pushPromotions(noisy, kForwardOffset, forwards);

            pushStandards(noisy, kLeftOffset, leftAttacks & theirs & ~kPromotionRank);
            pushStandards(noisy, kRightOffset, rightAttacks & theirs & ~kPromotionRank);
        }

        inline void generatePawnsNoisy(ScoredMoveList& noisy, const Position& pos, Bitboard dstMask) {
            if (pos.stm() == Color::kBlack) {
                generatePawnsNoisy_<Color::kBlack>(noisy, pos, dstMask);
            } else {
                generatePawnsNoisy_<Color::kWhite>(noisy, pos, dstMask);
            }
        }

        template <Color kUs>
        void generatePawnsQuiet_(ScoredMoveList& quiet, const BitboardSet& bbs, Bitboard dstMask, Bitboard occ) {
            static constexpr auto kPromotionRank = boards::promotionRank<kUs>();
            static constexpr auto kForwardOffset = offsets::up<kUs>();

            const auto forwardDstMask = dstMask & ~kPromotionRank & ~occ;

            const auto pawns = bbs.pawns<kUs>();

            const auto forwards = pawns.template shiftUpRelative<kUs>() & forwardDstMask;
            pushStandards(quiet, kForwardOffset, forwards);
        }

        inline void generatePawnsQuiet(ScoredMoveList& quiet, const Position& pos, Bitboard dstMask, Bitboard occ) {
            if (pos.stm() == Color::kBlack) {
                generatePawnsQuiet_<Color::kBlack>(quiet, pos.bbs(), dstMask, occ);
            } else {
                generatePawnsQuiet_<Color::kWhite>(quiet, pos.bbs(), dstMask, occ);
            }
        }

        template <PieceType kPiece, const std::array<Bitboard, 64>& kAttacks>
        inline void precalculated(ScoredMoveList& dst, const Position& pos, Bitboard dstMask) {
            const auto us = pos.stm();

            auto pieces = pos.bbs().forPiece(kPiece, us);
            while (!pieces.empty()) {
                const auto srcSquare = pieces.popLowestSquare();
                const auto attacks = kAttacks[static_cast<usize>(srcSquare)];

                pushStandards(dst, srcSquare, attacks & dstMask);
            }
        }

        void generateAlfils(ScoredMoveList& dst, const Position& pos, Bitboard dstMask) {
            precalculated<PieceType::kAlfil, attacks::kAlfilAttacks>(dst, pos, dstMask);
        }

        void generateFerzes(ScoredMoveList& dst, const Position& pos, Bitboard dstMask) {
            precalculated<PieceType::kFerz, attacks::kFerzAttacks>(dst, pos, dstMask);
        }

        void generateKnights(ScoredMoveList& dst, const Position& pos, Bitboard dstMask) {
            precalculated<PieceType::kKnight, attacks::kKnightAttacks>(dst, pos, dstMask);
        }

        void generateKings(ScoredMoveList& dst, const Position& pos, Bitboard dstMask) {
            precalculated<PieceType::kKing, attacks::kKingAttacks>(dst, pos, dstMask);
        }

        void generateRooks(ScoredMoveList& dst, const Position& pos, Bitboard dstMask) {
            const auto& bbs = pos.bbs();

            const auto us = pos.stm();
            const auto them = oppColor(us);

            const auto ours = bbs.forColor(us);
            const auto theirs = bbs.forColor(them);

            const auto occupancy = ours | theirs;

            auto rooks = bbs.rooks(us);
            while (!rooks.empty()) {
                const auto src = rooks.popLowestSquare();
                const auto attacks = attacks::getRookAttacks(src, occupancy);

                pushStandards(dst, src, attacks & dstMask);
            }
        }
    } // namespace

    void generateNoisy(ScoredMoveList& noisy, const Position& pos) {
        const auto& bbs = pos.bbs();

        const auto us = pos.stm();
        const auto them = oppColor(us);

        const auto ours = bbs.forColor(us);

        const auto kingDstMask = bbs.forColor(them);

        auto dstMask = kingDstMask;

        // promotions are noisy
        const auto promos = ~ours & (us == Color::kBlack ? boards::kRank1 : boards::kRank8);

        auto pawnDstMask = kingDstMask | promos;

        if (pos.isCheck()) {
            if (pos.checkers().multiple()) {
                generateKings(noisy, pos, kingDstMask);
                return;
            }

            dstMask = pos.checkers();
            pawnDstMask = kingDstMask | (promos & orthoRayBetween(pos.king(us), pos.checkers().lowestSquare()));
        }

        generateAlfils(noisy, pos, dstMask);
        generateFerzes(noisy, pos, dstMask);
        generateRooks(noisy, pos, dstMask);
        generatePawnsNoisy(noisy, pos, pawnDstMask);
        generateKnights(noisy, pos, dstMask);
        generateKings(noisy, pos, kingDstMask);
    }

    void generateQuiet(ScoredMoveList& quiet, const Position& pos) {
        const auto& bbs = pos.bbs();

        const auto us = pos.stm();
        const auto them = oppColor(us);

        const auto ours = bbs.forColor(us);
        const auto theirs = bbs.forColor(them);

        const auto kingDstMask = ~(ours | theirs);

        auto dstMask = kingDstMask;

        if (pos.isCheck()) {
            if (pos.checkers().multiple()) {
                generateKings(quiet, pos, kingDstMask);
                return;
            }

            dstMask = orthoRayBetween(pos.king(us), pos.checkers().lowestSquare());
        }

        generateAlfils(quiet, pos, dstMask);
        generateFerzes(quiet, pos, dstMask);
        generateRooks(quiet, pos, dstMask);
        generatePawnsQuiet(quiet, pos, dstMask, ours | theirs);
        generateKnights(quiet, pos, dstMask);
        generateKings(quiet, pos, kingDstMask);
    }

    void generateAll(ScoredMoveList& dst, const Position& pos) {
        const auto& bbs = pos.bbs();

        const auto us = pos.stm();

        const auto kingDstMask = ~bbs.forColor(pos.stm());

        auto dstMask = kingDstMask;

        if (pos.isCheck()) {
            if (pos.checkers().multiple()) {
                generateKings(dst, pos, kingDstMask);
                return;
            }

            dstMask = pos.checkers() | orthoRayBetween(pos.king(us), pos.checkers().lowestSquare());
        }

        generateAlfils(dst, pos, dstMask);
        generateFerzes(dst, pos, dstMask);
        generateRooks(dst, pos, dstMask);
        generatePawnsNoisy(dst, pos, dstMask);
        generatePawnsQuiet(dst, pos, dstMask, bbs.occupancy());
        generateKnights(dst, pos, dstMask);
        generateKings(dst, pos, kingDstMask);
    }
} // namespace oranj
