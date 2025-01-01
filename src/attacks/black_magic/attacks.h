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

#include "../../core.h"
#include "../../bitboard.h"
#include "../util.h"
#include "data.h"
#include "../../util/bits.h"

namespace oranj::attacks
{
	extern const std::array<Bitboard, black_magic::RookData.tableSize> RookAttacks;

	[[nodiscard]] inline auto getRookIdx(Bitboard occupancy, Square src)
	{
		const auto s = static_cast<i32>(src);

		const auto &data = black_magic::RookData.data[s];

		const auto magic = black_magic::Magics[s];
		const auto shift = black_magic::Shifts[s];

		return ((occupancy | data.mask) * magic) >> shift;
	}

	[[nodiscard]] inline auto getRookAttacks(Square src, Bitboard occupancy)
	{
		const auto s = static_cast<i32>(src);

		const auto &data = black_magic::RookData.data[s];
		const auto idx = getRookIdx(occupancy, src);

		return RookAttacks[data.offset + idx];
	}
}
