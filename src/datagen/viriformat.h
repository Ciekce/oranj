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

#include "common.h"
#include "format.h"
#include "marlinformat.h"

namespace oranj::datagen
{
	// Format originally from Viridithas
	// https://github.com/cosmobobak/viridithas/blob/029672a/src/datagen/dataformat.rs
	class Viriformat
	{
	public:
		Viriformat();
		~Viriformat() = default;

		static constexpr auto Extension = "vf";

		auto start(const Position &initialPosition) -> void;
		auto push(bool filtered, Move move, Score score) -> void;
		auto writeAllWithOutcome(std::ostream &stream, Outcome outcome) -> usize;

	private:
		using ScoredMove = std::pair<u16, i16>;
		static_assert(sizeof(ScoredMove) == sizeof(u16) + sizeof(i16));

		marlinformat::PackedBoard m_initial{};
		std::vector<ScoredMove> m_moves{};
	};

	static_assert(OutputFormat<Viriformat>);
}
