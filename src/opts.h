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

#include "wdl.h"
#include "util/range.h"

namespace oranj
{
	namespace opts
	{
		constexpr u32 DefaultThreadCount = 1;
		constexpr auto ThreadCountRange = util::Range<u32>{1,  2048};

		constexpr i32 DefaultNormalizedContempt = 0;

		struct GlobalOptions
		{
			u32 threads{DefaultThreadCount};

			bool chess960{false};
			bool showWdl{true};
			bool showCurrMove{false};

			bool softNodes{false};
			i32 softNodeHardLimitMultiplier{1678};

			bool enableWeirdTcs{false};

			i32 contempt{wdl::unnormalizeScoreMaterial58(DefaultNormalizedContempt)};
		};

		auto mutableOpts() -> GlobalOptions &;
	}

	extern const opts::GlobalOptions &g_opts;
}
