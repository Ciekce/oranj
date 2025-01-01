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

#include "../core.h"
#include "../bitboard.h"
#include "../util/cemath.h"

namespace oranj::attacks
{
	namespace internal
	{
		constexpr auto edges(i32 dir)
		{
			switch (dir)
			{
			case offsets::       Up: return boards::Rank8;
			case offsets::     Down: return boards::Rank1;
			case offsets::     Left: return boards::FileA;
			case offsets::    Right: return boards::FileH;
			default: __builtin_unreachable(); // don't
			}
		}

		constexpr auto generateSlidingAttacks(Square src, i32 dir, Bitboard occupancy)
		{
			Bitboard dst{};

			auto blockers = edges(dir);

			const bool right = dir < 0;
			const auto shift = util::abs(dir);

			auto bit = squareBit(src);

			if (!(blockers & bit).empty())
				return dst;

			blockers |= occupancy;

			do
			{
				if (right)
					dst |= bit >>= shift;
				else dst |= bit <<= shift;
			} while (!(bit & blockers));

			return dst;
		}
	}

	constexpr auto EmptyBoardRooks = []
	{
		std::array<Bitboard, 64> dst{};

		for (i32 square = 0; square < 64; ++square)
		{
			for (const auto dir : {
				offsets::Up,
				offsets::Down,
				offsets::Left,
				offsets::Right
			})
			{
				const auto attacks = internal::generateSlidingAttacks(static_cast<Square>(square), dir, 0);
				dst[square] |= attacks;
			}
		}

		return dst;
	}();

	consteval auto genRookAttacks(Square src, Bitboard occupancy)
	{
		Bitboard dst{};

		for (const auto dir : {
			offsets::Up,
			offsets::Down,
			offsets::Left,
			offsets::Right
		})
		{
			dst |= internal::generateSlidingAttacks(src, dir, occupancy);
		}

		return dst;
	}
}
