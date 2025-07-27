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

#include "move.h"

#include "opts.h"
#include "position/position.h"

fmt::format_context::iterator fmt::formatter<oranj::Move>::format(oranj::Move value, format_context& ctx) const {
    using namespace oranj;

    if (value.isNull()) {
        return format_to(ctx.out(), "????");
    }

    format_to(ctx.out(), "{}{}", value.fromSq(), value.toSq());

    if (value.isPromo()) {
        format_to(ctx.out(), "q");
    }

    return ctx.out();
}
