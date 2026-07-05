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

#include "pv.h"

#include <algorithm>

namespace oranj::search {
    void PvList::update(Move move, const PvList& child) {
        moves[0] = move;
        std::copy_n(child.moves.begin(), child.length, moves.begin() + 1);

        length = child.length + 1;

        assert(length == 1 || moves[0] != moves[1]);
    }

    void PvList::reset() {
        moves[0] = kNullMove;
        length = 0;
    }
} // namespace oranj::search
