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
#include <cassert>

#include "../core.h"
#include "../bitboard.h"
#include "util.h"
#include "../util/bits.h"

#if OJ_HAS_BMI2
#include "bmi2/attacks.h"
#else
#include "black_magic/attacks.h"
#endif

namespace oranj::attacks
{
	constexpr auto AlfilAttacks = []
	{
		std::array<Bitboard, 64> dst{};

		for (usize i = 0; i < dst.size(); ++i)
		{
			const auto bit = Bitboard::fromSquare(static_cast<Square>(i));

			auto &attacks = dst[i];

			attacks |= bit.shiftUpLeft().shiftUpLeft();
			attacks |= bit.shiftUpRight().shiftUpRight();
			attacks |= bit.shiftDownLeft().shiftDownLeft();
			attacks |= bit.shiftDownRight().shiftDownRight();
		}

		return dst;
	}();

	constexpr auto FerzAttacks = []
	{
		std::array<Bitboard, 64> dst{};

		for (usize i = 0; i < dst.size(); ++i)
		{
			const auto bit = Bitboard::fromSquare(static_cast<Square>(i));

			auto &attacks = dst[i];

			attacks |= bit.shiftUpLeft();
			attacks |= bit.shiftUpRight();
			attacks |= bit.shiftDownLeft();
			attacks |= bit.shiftDownRight();
		}

		return dst;
	}();

	constexpr auto KnightAttacks = []
	{
		std::array<Bitboard, 64> dst{};

		for (usize i = 0; i < dst.size(); ++i)
		{
			const auto bit = Bitboard::fromSquare(static_cast<Square>(i));

			auto &attacks = dst[i];

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

	constexpr auto KingAttacks = []
	{
		std::array<Bitboard, 64> dst{};

		for (usize i = 0; i < dst.size(); ++i)
		{
			const auto bit = Bitboard::fromSquare(static_cast<Square>(i));

			auto &attacks = dst[i];

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

	template <Color Us>
	consteval auto generatePawnAttacks()
	{
		std::array<Bitboard, 64> dst{};

		for (usize i = 0; i < dst.size(); ++i)
		{
			const auto bit = Bitboard::fromSquare(static_cast<Square>(i));

			dst[i] |= bit.shiftUpLeftRelative<Us>();
			dst[i] |= bit.shiftUpRightRelative<Us>();
		}

		return dst;
	}

	constexpr auto BlackPawnAttacks = generatePawnAttacks<Color::Black>();
	constexpr auto WhitePawnAttacks = generatePawnAttacks<Color::White>();

	constexpr auto getAlfilAttacks(Square src)
	{
		return AlfilAttacks[static_cast<usize>(src)];
	}

	constexpr auto getFerzAttacks(Square src)
	{
		return FerzAttacks[static_cast<usize>(src)];
	}

	constexpr auto getKnightAttacks(Square src)
	{
		return KnightAttacks[static_cast<usize>(src)];
	}

	constexpr auto getKingAttacks(Square src)
	{
		return KingAttacks[static_cast<usize>(src)];
	}

	constexpr auto getPawnAttacks(Square src, Color color)
	{
		const auto &attacks = color == Color::White ? WhitePawnAttacks : BlackPawnAttacks;
		return attacks[static_cast<usize>(src)];
	}

	inline auto getNonPawnPieceAttacks(PieceType piece, Square src, Bitboard occupancy = Bitboard{})
	{
		assert(piece != PieceType::None);
		assert(piece != PieceType::Pawn);

		switch (piece)
		{
		case PieceType::Alfil: return getAlfilAttacks(src);
		case PieceType::Ferz: return getFerzAttacks(src);
		case PieceType::Knight: return getKnightAttacks(src);
		case PieceType::Rook: return getRookAttacks(src, occupancy);
		case PieceType::King: return getKingAttacks(src);
		default: __builtin_unreachable();
		}
	}
}
