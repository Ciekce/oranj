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

#include "core.h"

fmt::format_context::iterator fmt::formatter<oranj::Piece>::format(oranj::Piece value, format_context& ctx) const {
    using namespace oranj;

    static constexpr std::array kPieceChars = {
        'p', // black pawn
        'P', // white pawn
        'b', // black alfil
        'B', // white alfil
        'q', // black ferz
        'Q', // white ferz
        'n', // black knight
        'N', // white knight
        'r', // black rook
        'R', // white rook
        'k', // black king
        'K', // white king
        ' ', // none
    };

    if (value.idx() >= kPieceChars.size()) {
        return format_to(ctx.out(), "?");
    }

    return format_to(ctx.out(), "{}", kPieceChars[value.idx()]);
}

fmt::format_context::iterator fmt::formatter<oranj::PieceType>::format(
    oranj::PieceType value,
    format_context& ctx
) const {
    using namespace oranj;

    static constexpr std::array kPieceTypeChars = {
        'p', // pawn
        'b', // alfil
        'q', // ferz
        'n', // knight
        'r', // rook
        'k', // king
        ' ', // none
    };

    if (value.idx() >= kPieceTypeChars.size()) {
        return format_to(ctx.out(), "?");
    }

    return format_to(ctx.out(), "{}", kPieceTypeChars[value.idx()]);
}

fmt::format_context::iterator fmt::formatter<oranj::Square>::format(oranj::Square value, format_context& ctx) const {
    using namespace oranj;

    return write_padded(ctx, [=, this](auto out) {
        if (value == Squares::kNone) {
            return format_to(out, "??");
        }

        return format_to(
            out,
            "{}{}",
            nested(static_cast<char>('a' + value.file())),
            nested(static_cast<char>('1' + value.rank()))
        );
    });
}
