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
#include <cstdint>

#include "core.h"
#include "opts.h"
#include "util/static_vector.h"

namespace oranj {
    enum class MoveType {
        kStandard = 0,
        kPromotion,
    };

    class Move {
    public:
        constexpr Move() = default;

        [[nodiscard]] constexpr usize fromSqIdx() const {
            return m_move >> 10;
        }

        [[nodiscard]] constexpr Square fromSq() const {
            return static_cast<Square>(fromSqIdx());
        }

        [[nodiscard]] constexpr i32 fromSqRank() const {
            return m_move >> 13;
        }

        [[nodiscard]] constexpr i32 fromSqFile() const {
            return (m_move >> 10) & 0x7;
        }

        [[nodiscard]] constexpr usize toSqIdx() const {
            return (m_move >> 4) & 0x3F;
        }

        [[nodiscard]] constexpr Square toSq() const {
            return static_cast<Square>(toSqIdx());
        }

        [[nodiscard]] constexpr i32 toSqRank() const {
            return (m_move >> 7) & 0x7;
        }

        [[nodiscard]] constexpr i32 toSqFile() const {
            return (m_move >> 4) & 0x7;
        }

        [[nodiscard]] constexpr MoveType type() const {
            return static_cast<MoveType>(m_move & 0x3);
        }

        [[nodiscard]] constexpr bool isPromo() const {
            return type() == MoveType::kPromotion;
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
            return Move{static_cast<u16>(
                (static_cast<u16>(src) << 10) | (static_cast<u16>(dst) << 4) | static_cast<u16>(MoveType::kStandard)
            )};
        }

        [[nodiscard]] static constexpr Move promotion(Square src, Square dst) {
            return Move{static_cast<u16>(
                (static_cast<u16>(src) << 10) | (static_cast<u16>(dst) << 4) | static_cast<u16>(MoveType::kPromotion)
            )};
        }

    private:
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
