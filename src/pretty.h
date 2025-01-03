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

#include <iostream>

#include "bitboard.h"
#include "position/position.h"

namespace oranj
{
	auto printBitboard(std::ostream &out, Bitboard board) -> void;
	auto printBitboardCompact(std::ostream &out, Bitboard board) -> void;

	auto printBoard(std::ostream &out, const Position &position) -> void;

	auto printScore(std::ostream &out, Score score) -> void;
}
