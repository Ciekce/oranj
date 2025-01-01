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
	extern const std::array<u16, bmi2::RookData.tableSize>   RookAttacks;

	inline auto getRookAttacks(Square src, Bitboard occupancy) -> Bitboard
	{
		const auto s = static_cast<i32>(src);

		const auto &data = bmi2::RookData.data[s];

		const auto idx = util::pext(occupancy, data.srcMask);
		const auto attacks = util::pdep(RookAttacks[data.offset + idx], data.dstMask);

		return attacks;
	}
}
