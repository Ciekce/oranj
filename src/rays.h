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

#include "types.h"

#include <array>

#include "core.h"
#include "bitboard.h"
#include "attacks/util.h"
#include "util/multi_array.h"

namespace oranj
{
	constexpr auto BetweenRays = []
	{
		util::MultiArray<Bitboard, 64, 64> dst{};

		for (i32 from = 0; from < 64; ++from)
		{
			const auto srcSquare = static_cast<Square>(from);
			const auto srcMask = squareBit(srcSquare);

			const auto rookAttacks = attacks::EmptyBoardRooks[from];

			for (i32 to = 0; to < 64; ++to)
			{
				if (from == to)
					continue;

				const auto dstSquare = static_cast<Square>(to);
				const auto dstMask = squareBit(dstSquare);

				if (rookAttacks[dstSquare])
					dst[from][to]
						= attacks::genRookAttacks(srcSquare, dstMask)
						& attacks::genRookAttacks(dstSquare, srcMask);
			}
		}

		return dst;
	}();

	constexpr auto IntersectingRays = []
	{
		util::MultiArray<Bitboard, 64, 64> dst{};

		for (i32 from = 0; from < 64; ++from)
		{
			const auto srcSquare = static_cast<Square>(from);
			const auto srcMask = squareBit(srcSquare);

			const auto rookAttacks = attacks::EmptyBoardRooks[from];

			for (i32 to = 0; to < 64; ++to)
			{
				if (from == to)
					continue;

				const auto dstSquare = static_cast<Square>(to);
				const auto dstMask = squareBit(dstSquare);

				if (rookAttacks[dstSquare])
					dst[from][to]
						= (srcMask | attacks::genRookAttacks(srcSquare, Bitboard{}))
						& (dstMask | attacks::genRookAttacks(dstSquare, Bitboard{}));
			}
		}

		return dst;
	}();

	constexpr auto orthoRayBetween(Square src, Square dst)
	{
		return BetweenRays[static_cast<i32>(src)][static_cast<i32>(dst)];
	}

	constexpr auto orthoRayIntersecting(Square src, Square dst)
	{
		return IntersectingRays[static_cast<i32>(src)][static_cast<i32>(dst)];
	}
}
