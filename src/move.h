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

#include "types.h"

#include "core.h"
#include "util/static_vector.h"

namespace oranj {
    class Move {
    public:
        constexpr Move() = default;

        [[nodiscard]] constexpr u16 raw() const {
            return m_move ^ kPresentBit;
        }

        [[nodiscard]] constexpr Square fromSq() const {
            const auto raw = (m_move >> kFromShift) & kSquareMask;
            return Square::fromRaw(raw);
        }

        [[nodiscard]] constexpr Square toSq() const {
            const auto raw = (m_move >> kToShift) & kSquareMask;
            return Square::fromRaw(raw);
        }

        [[nodiscard]] constexpr bool isPromo() const {
            const auto raw = (m_move >> kPromoShift) & kFlagMask;
            return raw != 0;
        }

        [[nodiscard]] constexpr bool isNull() const {
            return m_move == 0;
        }

        [[nodiscard]] constexpr u16 data() const {
            return m_move;
        }

        [[nodiscard]] explicit constexpr operator bool() const {
            return !isNull();
        }

        constexpr bool operator==(const Move& other) const = default;

        [[nodiscard]] static constexpr Move standard(Square src, Square dst) {
            auto value = kPresentBit;

            value |= src.raw() << kFromShift;
            value |= dst.raw() << kToShift;

            return Move{value};
        }

        [[nodiscard]] static constexpr Move promotion(Square src, Square dst) {
            auto value = kPresentBit;

            value |= src.raw() << kFromShift;
            value |= dst.raw() << kToShift;
            value |= 1 << kPromoShift;

            return Move{value};
        }

    private:
        static constexpr usize kSquareBits = 6;
        static constexpr usize kFlagBits = 1;

        static constexpr auto kTotalBits = kSquareBits * 2 + kFlagBits;

        static constexpr u16 kSquareMask = (1 << kSquareBits) - 1;
        static constexpr u16 kFlagMask = (1 << kFlagBits) - 1;

        static constexpr u16 kValidMask = (1 << kTotalBits) - 1;

        // Make a1a1 representable by always setting the msb internally
        static constexpr u16 kPresentBit = 1 << 15;

        static constexpr usize kFromShift = 0;
        static constexpr usize kToShift = 6;
        static constexpr usize kPromoShift = 12;

        explicit constexpr Move(u16 move) :
                m_move{move} {}

        u16 m_move{};
    };

    constexpr Move kNullMove{};

    // assumed upper bound for number of possible moves is 218
    constexpr usize kDefaultMoveListCapacity = 256;

    using MoveList = StaticVector<Move, kDefaultMoveListCapacity>;
} // namespace oranj

template <>
struct fmt::formatter<oranj::Move> : fmt::formatter<std::string_view> {
    format_context::iterator format(oranj::Move value, format_context& ctx) const;
};
