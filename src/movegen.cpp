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

#include "movegen.h"

#include "attacks/attacks.h"
#include "opts.h"
#include "rays.h"

namespace oranj {
    namespace {
        inline void pushStandards(ScoredMoveList& dst, i32 offset, Bitboard board) {
            for (const auto dstSquare : board) {
                const auto srcSquare = dstSquare.offset(-offset);
                dst.push({Move::standard(srcSquare, dstSquare), 0});
            }
        }

        inline void pushStandards(ScoredMoveList& dst, Square srcSquare, Bitboard board) {
            for (const auto dstSquare : board) {
                dst.push({Move::standard(srcSquare, dstSquare), 0});
            }
        }

        inline void pushPromotions(ScoredMoveList& noisy, i32 offset, Bitboard board) {
            for (const auto dstSquare : board) {
                const auto srcSquare = dstSquare.offset(-offset);
                noisy.push({Move::promotion(srcSquare, dstSquare), 0});
            }
        }

        void generatePawnsNoisy(ScoredMoveList& noisy, const Position& pos, Bitboard dstMask) {
            const auto us = pos.stm();
            const auto them = pos.nstm();

            const auto promotionRank = boards::promotionRank(us);

            const auto leftOffset = offsets::upLeft(us);
            const auto rightOffset = offsets::upRight(us);

            const auto king = pos.king(us);
            const auto leftPinMask =
                us == Colors::kBlack ? attacks::kDiagonals[king.idx()] : attacks::kAntiDiagonals[king.idx()];
            const auto rightPinMask =
                us == Colors::kBlack ? attacks::kAntiDiagonals[king.idx()] : attacks::kDiagonals[king.idx()];

            const auto theirs = pos.bb(them);

            const auto pawns = pos.bbs().pawns(us);
            const auto pinned = pos.pinned(us);

            const auto movablePawns = [&](Bitboard pinMask) { return (pawns & ~pinned) | (pawns & pinMask); };

            const auto leftAttacks = movablePawns(leftPinMask).shiftUpLeftRelative(us) & dstMask;
            const auto rightAttacks = movablePawns(rightPinMask).shiftUpRightRelative(us) & dstMask;

            pushPromotions(noisy, leftOffset, leftAttacks & theirs & promotionRank);
            pushPromotions(noisy, rightOffset, rightAttacks & theirs & promotionRank);

            pushStandards(noisy, leftOffset, leftAttacks & theirs & ~promotionRank);
            pushStandards(noisy, rightOffset, rightAttacks & theirs & ~promotionRank);
        }

        void generatePawnsQuiet(ScoredMoveList& quiet, const Position& pos, Bitboard dstMask, Bitboard occ) {
            const auto us = pos.stm();
            const auto them = pos.nstm();

            const auto promotionRank = boards::promotionRank(us);

            const auto forwardOffset = offsets::up(us);

            const auto king = pos.king(us);
            const auto forwardPinMask = Bitboard::file(king.file());

            const auto theirs = pos.bb(them);

            const auto forwardDstMask = dstMask & ~theirs;

            const auto pawns = pos.bbs().pawns(us);
            const auto pinned = pos.pinned(us);

            const auto movablePawns = [&](Bitboard pinMask) { return (pawns & ~pinned) | (pawns & pinMask); };

            const auto pushes = movablePawns(forwardPinMask).shiftUpRelative(us) & ~occ & forwardDstMask;

            pushPromotions(quiet, forwardOffset, pushes & promotionRank);
            pushStandards(quiet, forwardOffset, pushes & ~promotionRank);
        }

        void generateLeapers(ScoredMoveList& dst, const Position& pos, Bitboard dstMask) {
            const auto pinned = pos.pinned(pos.stm());

            const auto alfils = pos.bbs().alfils(pos.stm());
            for (const auto srcSquare : alfils & ~pinned) {
                const auto attacks = attacks::kAlfilAttacks[srcSquare.idx()];
                pushStandards(dst, srcSquare, attacks & dstMask);
            }

            const auto ferzes = pos.bbs().ferzes(pos.stm());
            for (const auto srcSquare : ferzes & ~pinned) {
                const auto attacks = attacks::kFerzAttacks[srcSquare.idx()];
                pushStandards(dst, srcSquare, attacks & dstMask);
            }

            const auto knights = pos.bbs().knights(pos.stm());
            for (const auto srcSquare : knights & ~pinned) {
                const auto attacks = attacks::kKnightAttacks[srcSquare.idx()];
                pushStandards(dst, srcSquare, attacks & dstMask);
            }
        }

        void generateKings(ScoredMoveList& dst, const Position& pos, Bitboard dstMask) {
            const auto king = pos.king(pos.stm());
            const auto attacks = attacks::kKingAttacks[king.idx()];
            pushStandards(dst, king, attacks & dstMask & ~pos.threats());
        }

        void generateRooks(ScoredMoveList& dst, const Position& pos, Bitboard dstMask) {
            const auto& bbs = pos.bbs();

            const auto us = pos.stm();

            const auto occ = pos.occ();
            const auto pinned = pos.pinned(us);

            const auto king = pos.king(us);

            const auto rooks = bbs.rooks(us);

            for (const auto src : rooks & ~pinned) {
                const auto attacks = attacks::getRookAttacks(src, occ);
                pushStandards(dst, src, attacks & dstMask);
            }

            for (const auto src : rooks& pinned) {
                const auto pinRay = rayPast(king, src);
                const auto attacks = attacks::getRookAttacks(src, occ);
                pushStandards(dst, src, attacks & dstMask & pinRay);
            }
        }
    } // namespace

    void generateNoisy(ScoredMoveList& noisy, const Position& pos) {
        const auto us = pos.stm();
        const auto them = us.flip();

        const auto kingDstMask = pos.bb(them);

        auto dstMask = kingDstMask;

        if (pos.isCheck()) {
            if (pos.checkers().multiple()) {
                generateKings(noisy, pos, kingDstMask);
                return;
            }

            dstMask = pos.checkers();
        }

        generatePawnsNoisy(noisy, pos, dstMask);
        generateLeapers(noisy, pos, dstMask);
        generateRooks(noisy, pos, dstMask);
        generateKings(noisy, pos, kingDstMask);
    }

    void generateQuiet(ScoredMoveList& quiet, const Position& pos) {
        const auto us = pos.stm();
        const auto them = us.flip();

        const auto ours = pos.bb(us);
        const auto theirs = pos.bb(them);

        const auto kingDstMask = ~(ours | theirs);

        auto dstMask = kingDstMask;

        if (pos.isCheck()) {
            if (pos.checkers().multiple()) {
                generateKings(quiet, pos, kingDstMask);
                return;
            }

            dstMask = rayBetween(pos.king(us), pos.checkers().lowestSquare());
        }

        generatePawnsQuiet(quiet, pos, dstMask, ours | theirs);
        generateLeapers(quiet, pos, dstMask);
        generateRooks(quiet, pos, dstMask);
        generateKings(quiet, pos, kingDstMask);
    }

    void generateAll(ScoredMoveList& dst, const Position& pos) {
        const auto us = pos.stm();

        const auto kingDstMask = ~pos.bb(pos.stm());

        auto dstMask = kingDstMask;

        if (pos.isCheck()) {
            if (pos.checkers().multiple()) {
                generateKings(dst, pos, kingDstMask);
                return;
            }

            dstMask = pos.checkers() | rayBetween(pos.king(us), pos.checkers().lowestSquare());
        }

        generatePawnsNoisy(dst, pos, dstMask);
        generatePawnsQuiet(dst, pos, dstMask, pos.occ());
        generateLeapers(dst, pos, dstMask);
        generateRooks(dst, pos, dstMask);
        generateKings(dst, pos, kingDstMask);
    }
} // namespace oranj
