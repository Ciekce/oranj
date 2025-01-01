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

#include <array>
#include <algorithm>

#include "attacks/attacks.h"
#include "rays.h"
#include "util/bitfield.h"
#include "opts.h"

namespace oranj
{
	namespace
	{
		inline auto pushStandards(ScoredMoveList &dst, i32 offset, Bitboard board)
		{
			while (!board.empty())
			{
				const auto dstSquare = board.popLowestSquare();
				const auto srcSquare = static_cast<Square>(static_cast<i32>(dstSquare) - offset);

				dst.push({Move::standard(srcSquare, dstSquare), 0});
			}
		}

		inline auto pushStandards(ScoredMoveList &dst, Square srcSquare, Bitboard board)
		{
			while (!board.empty())
			{
				const auto dstSquare = board.popLowestSquare();
				dst.push({Move::standard(srcSquare, dstSquare), 0});
			}
		}

		inline auto pushPromotions(ScoredMoveList &noisy, i32 offset, Bitboard board)
		{
			while (!board.empty())
			{
				const auto dstSquare = board.popLowestSquare();
				const auto srcSquare = static_cast<Square>(static_cast<i32>(dstSquare) - offset);

				noisy.push({Move::promotion(srcSquare, dstSquare), 0});
			}
		}

		template <Color Us>
		auto generatePawnsNoisy_(ScoredMoveList &noisy, const Position &pos, Bitboard dstMask)
		{
			constexpr auto Them = oppColor(Us);

			constexpr auto PromotionRank = boards::promotionRank<Us>();

			constexpr auto ForwardOffset = offsets::up<Us>();
			constexpr auto LeftOffset = offsets::upLeft<Us>();
			constexpr auto RightOffset = offsets::upRight<Us>();

			const auto &bbs = pos.bbs();

			const auto theirs = bbs.occupancy<Them>();

			const auto forwardDstMask = dstMask & PromotionRank & ~theirs;

			const auto pawns = bbs.pawns<Us>();

			const auto leftAttacks = pawns.template shiftUpLeftRelative<Us>() & dstMask;
			const auto rightAttacks = pawns.template shiftUpRightRelative<Us>() & dstMask;

			pushPromotions(noisy, LeftOffset,   leftAttacks & theirs & PromotionRank);
			pushPromotions(noisy, RightOffset, rightAttacks & theirs & PromotionRank);

			const auto forwards = pawns.template shiftUpRelative<Us>() & forwardDstMask;
			pushPromotions(noisy, ForwardOffset, forwards);

			pushStandards(noisy,  LeftOffset,  leftAttacks & theirs & ~PromotionRank);
			pushStandards(noisy, RightOffset, rightAttacks & theirs & ~PromotionRank);
		}

		inline auto generatePawnsNoisy(ScoredMoveList &noisy, const Position &pos, Bitboard dstMask)
		{
			if (pos.toMove() == Color::Black)
				generatePawnsNoisy_<Color::Black>(noisy, pos, dstMask);
			else generatePawnsNoisy_<Color::White>(noisy, pos, dstMask);
		}

		template <Color Us>
		auto generatePawnsQuiet_(ScoredMoveList &quiet, const BitboardSet &bbs, Bitboard dstMask, Bitboard occ)
		{
			constexpr auto PromotionRank = boards::promotionRank<Us>();

			const auto ForwardOffset = offsets::up<Us>();

			const auto forwardDstMask = dstMask & ~PromotionRank & ~occ;

			const auto pawns = bbs.pawns<Us>();

			const auto forwards = pawns.template shiftUpRelative<Us>() & forwardDstMask;
			pushStandards(quiet, ForwardOffset, forwards);
		}

		inline auto generatePawnsQuiet(ScoredMoveList &quiet, const Position &pos, Bitboard dstMask, Bitboard occ)
		{
			if (pos.toMove() == Color::Black)
				generatePawnsQuiet_<Color::Black>(quiet, pos.bbs(), dstMask, occ);
			else generatePawnsQuiet_<Color::White>(quiet, pos.bbs(), dstMask, occ);
		}

		template <PieceType Piece, const std::array<Bitboard, 64> &Attacks>
		inline auto precalculated(ScoredMoveList &dst, const Position &pos, Bitboard dstMask)
		{
			const auto us = pos.toMove();

			auto pieces = pos.bbs().forPiece(Piece, us);
			while (!pieces.empty())
			{
				const auto srcSquare = pieces.popLowestSquare();
				const auto attacks = Attacks[static_cast<usize>(srcSquare)];

				pushStandards(dst, srcSquare, attacks & dstMask);
			}
		}

		auto generateAlfils(ScoredMoveList &dst, const Position &pos, Bitboard dstMask)
		{
			precalculated<PieceType::Alfil, attacks::AlfilAttacks>(dst, pos, dstMask);
		}

		auto generateFerzes(ScoredMoveList &dst, const Position &pos, Bitboard dstMask)
		{
			precalculated<PieceType::Ferz, attacks::FerzAttacks>(dst, pos, dstMask);
		}

		auto generateKnights(ScoredMoveList &dst, const Position &pos, Bitboard dstMask)
		{
			precalculated<PieceType::Knight, attacks::KnightAttacks>(dst, pos, dstMask);
		}

		auto generateKings(ScoredMoveList &dst, const Position &pos, Bitboard dstMask)
		{
			precalculated<PieceType::King, attacks::KingAttacks>(dst, pos, dstMask);
		}

		auto generateRooks(ScoredMoveList &dst, const Position &pos, Bitboard dstMask)
		{
			const auto &bbs = pos.bbs();

			const auto us = pos.toMove();
			const auto them = oppColor(us);

			const auto ours = bbs.forColor(us);
			const auto theirs = bbs.forColor(them);

			const auto occupancy = ours | theirs;

			auto rooks = bbs.rooks(us);

			while (!rooks.empty())
			{
				const auto src = rooks.popLowestSquare();
				const auto attacks = attacks::getRookAttacks(src, occupancy);

				pushStandards(dst, src, attacks & dstMask);
			}
		}
	}

	auto generateNoisy(ScoredMoveList &noisy, const Position &pos) -> void
	{
		const auto &bbs = pos.bbs();

		const auto us = pos.toMove();
		const auto them = oppColor(us);

		const auto ours = bbs.forColor(us);

		const auto kingDstMask = bbs.forColor(them);

		auto dstMask = kingDstMask;

		// promotions are noisy
		const auto promos = ~ours & (us == Color::Black ? boards::Rank1 : boards::Rank8);

		auto pawnDstMask = kingDstMask | promos;

		if (pos.isCheck())
		{
			if (pos.checkers().multiple())
			{
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

	auto generateQuiet(ScoredMoveList &quiet, const Position &pos) -> void
	{
		const auto &bbs = pos.bbs();

		const auto us = pos.toMove();
		const auto them = oppColor(us);

		const auto ours = bbs.forColor(us);
		const auto theirs = bbs.forColor(them);

		const auto kingDstMask = ~(ours | theirs);

		auto dstMask = kingDstMask;

		if (pos.isCheck())
		{
			if (pos.checkers().multiple())
			{
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

	auto generateAll(ScoredMoveList &dst, const Position &pos) -> void
	{
		const auto &bbs = pos.bbs();

		const auto us = pos.toMove();

		const auto kingDstMask = ~bbs.forColor(pos.toMove());

		auto dstMask = kingDstMask;

		if (pos.isCheck())
		{
			if (pos.checkers().multiple())
			{
				generateKings(dst, pos, kingDstMask);
				return;
			}

			dstMask = pos.checkers()
				| orthoRayBetween(pos.king(us), pos.checkers().lowestSquare());
		}

		generateAlfils(dst, pos, dstMask);
		generateFerzes(dst, pos, dstMask);
		generateRooks(dst, pos, dstMask);
		generatePawnsNoisy(dst, pos, dstMask);
		generatePawnsQuiet(dst, pos, dstMask, bbs.occupancy());
		generateKnights(dst, pos, dstMask);
		generateKings(dst, pos, kingDstMask);
	}
}
