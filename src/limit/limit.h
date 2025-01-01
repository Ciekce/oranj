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

#include "../search_fwd.h"

namespace oranj::limit
{
	class ISearchLimiter
	{
	public:
		virtual ~ISearchLimiter() = default;

		virtual auto update(const search::SearchData &data, Score score, Move bestMove, usize totalNodes) -> void {}
		virtual auto updateMoveNodes(Move move, usize nodes) -> void {}

		[[nodiscard]] virtual auto stop(const search::SearchData &data, bool allowSoftTimeout) -> bool = 0;

		[[nodiscard]] virtual auto stopped() const -> bool = 0;
	};
}
