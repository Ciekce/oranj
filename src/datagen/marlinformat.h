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

#include <vector>

#include "format.h"
#include "../position/position.h"
#include "../util/u4array.h"

namespace oranj::datagen
{
	namespace marlinformat
	{
		// https://github.com/jnlt3/marlinflow/blob/main/marlinformat/src/lib.rs
		struct __attribute__((packed)) PackedBoard
		{
			u64 occupancy;
			util::U4Array<32> pieces;
			u8 stmEpSquare;
			u8 halfmoveClock;
			u16 fullmoveNumber;
			i16 eval;
			Outcome wdl;
			[[maybe_unused]] u8 extra;

			[[nodiscard]] static auto pack(const Position &pos, i16 score)
			{
				static constexpr u8 UnmovedRook = 6;

				PackedBoard board{};

				const auto &boards = pos.boards();

				auto occupancy = boards.bbs().occupancy();
				board.occupancy = occupancy;

				usize i = 0;
				while (occupancy)
				{
					const auto square = occupancy.popLowestSquare();
					const auto piece = boards.pieceAt(square);

					const auto pieceId = static_cast<u8>(pieceType(piece));
					const u8 colorId = pieceColor(piece) == Color::Black ? (1 << 3) : 0;

					board.pieces[i++] = pieceId | colorId;
				}

				const u8 stm = pos.toMove() == Color::Black ? (1 << 7) : 0;

				board.stmEpSquare = stm;
				board.halfmoveClock = pos.halfmove();
				board.fullmoveNumber = pos.fullmove();
				board.eval = score;

				return board;
			}
		};
	}

	class Marlinformat
	{
	public:
		Marlinformat();
		~Marlinformat() = default;

		static constexpr auto Extension = "bin";

		auto start(const Position &initialPosition) -> void;
		auto push(bool filtered, Move move, Score score) -> void;
		auto writeAllWithOutcome(std::ostream &stream, Outcome outcome) -> usize;

	private:
		std::vector<marlinformat::PackedBoard> m_positions{};
		Position m_curr;
	};

	static_assert(OutputFormat<Marlinformat>);
}
