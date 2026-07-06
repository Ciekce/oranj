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

#include "oranjformat.h"

#include <array>

namespace oranj::datagen {
    Oranjformat::Oranjformat() {
        m_moves.reserve(256);
    }

    void Oranjformat::start(const Position& initialPosition) {
        m_initial = marlinformat::PackedBoard::pack(initialPosition, 0);
        m_moves.clear();
    }

    void Oranjformat::push(bool filtered, Move move, Score score) {
        OJ_UNUSED(filtered);
        m_moves.push_back({move.raw(), static_cast<i16>(score)});
    }

    usize Oranjformat::writeAllWithOutcome(std::ostream& stream, Outcome outcome) {
        static constexpr std::array<u8, sizeof(ScoredMove)> kNullTerminator{};

        m_initial.wdl = outcome;

        stream.write(reinterpret_cast<const char*>(&m_initial), sizeof(marlinformat::PackedBoard));
        stream.write(reinterpret_cast<const char*>(m_moves.data()), sizeof(ScoredMove) * m_moves.size());
        stream.write(reinterpret_cast<const char*>(kNullTerminator.data()), sizeof(ScoredMove));

        return m_moves.size() + 1;
    }
} // namespace oranj::datagen
