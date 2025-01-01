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
#include "util/rng.h"

namespace oranj::keys
{
	namespace sizes
	{
		constexpr usize PieceSquares = 12 * 64;
		constexpr usize Color = 1;

		constexpr auto Total = PieceSquares + Color;
	}

	namespace offsets
	{
		constexpr usize PieceSquares = 0;
		constexpr auto Color = PieceSquares + sizes::PieceSquares;
	}

	constexpr auto Keys = []
	{
		constexpr auto Seed = U64(0xD06C659954EC904A);

		std::array<u64, sizes::Total> keys{};

		util::rng::Jsf64Rng rng{Seed};

		for (auto &key : keys)
		{
			key = rng.nextU64();
		}

		return keys;
	}();

	inline auto pieceSquare(Piece piece, Square square) -> u64
	{
		if (piece == Piece::None || square == Square::None)
			return 0;

		return Keys[offsets::PieceSquares + static_cast<usize>(square) * 12 + static_cast<usize>(piece)];
	}

	// for flipping
	inline auto color()
	{
		return Keys[offsets::Color];
	}

	inline auto color(Color c)
	{
		return c == Color::White ? 0 : color();
	}
}
