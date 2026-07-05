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

#ifndef OJ_USE_LIBNUMA

    #include "numa.h"

namespace oranj::numa {
    bool init() {
        return true;
    }

    void bindThread(u32 numaId) {
        OJ_UNUSED(numaId);
    }

    i32 nodeCount() {
        return 1;
    }
} // namespace oranj::numa
#endif
