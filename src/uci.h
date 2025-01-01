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

#include <string>
#include <span>

#include "core.h"
#include "move.h"
#include "tunable.h"

namespace oranj::uci
{
	constexpr auto ContemptRange = util::Range<i32>{-1000, 1000};

	auto run() -> i32;

	[[nodiscard]] auto moveToString(Move move) -> std::string;

#if OJ_EXTERNAL_TUNE
	auto printWfTuningParams(std::span<const std::string> params) -> void;
	auto printCttTuningParams(std::span<const std::string> params) -> void;
	auto printObTuningParams(std::span<const std::string> params) -> void;
#endif
}
