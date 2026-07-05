/*
 * oranj, a UCI shatranj engine
 * Copyright (C) 2026 Ciekce
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

#include <span>
#include <string_view>

#include "../tunable.h"

namespace oranj::uci {
    i32 run(std::span<const std::string_view> commands = {});

#if OJ_EXTERNAL_TUNE
    void printWfTuningParams(std::span<const std::string_view> params);
    void printCttTuningParams(std::span<const std::string_view> params);
    void printObTuningParams(std::span<const std::string_view> params);
#endif
} // namespace oranj::uci
