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

#include "position.h"

#include <algorithm>
#include <cassert>

#include "../attacks/attacks.h"
#include "../cuckoo.h"
#include "../movegen.h"
#include "../opts.h"
#include "../rays.h"
#include "../util/parse.h"
#include "../util/split.h"

namespace oranj {
    template Position Position::applyMove<NnueUpdateAction::kNone>(Move, eval::NnueState*) const;
    template Position Position::applyMove<NnueUpdateAction::kQueue>(Move, eval::NnueState*) const;
    template Position Position::applyMove<NnueUpdateAction::kApply>(Move, eval::NnueState*) const;

    template void Position::setPiece<false>(Piece, Square);
    template void Position::setPiece<true>(Piece, Square);

    template void Position::removePiece<false>(Piece, Square);
    template void Position::removePiece<true>(Piece, Square);

    template void Position::movePieceNoCap<false>(Piece, Square, Square);
    template void Position::movePieceNoCap<true>(Piece, Square, Square);

    template Piece Position::movePiece<false, false>(Piece, Square, Square, eval::NnueUpdates&);
    template Piece Position::movePiece<true, false>(Piece, Square, Square, eval::NnueUpdates&);
    template Piece Position::movePiece<false, true>(Piece, Square, Square, eval::NnueUpdates&);
    template Piece Position::movePiece<true, true>(Piece, Square, Square, eval::NnueUpdates&);

    template Piece Position::promotePawn<false, false>(Piece, Square, Square, eval::NnueUpdates&);
    template Piece Position::promotePawn<true, false>(Piece, Square, Square, eval::NnueUpdates&);
    template Piece Position::promotePawn<false, true>(Piece, Square, Square, eval::NnueUpdates&);
    template Piece Position::promotePawn<true, true>(Piece, Square, Square, eval::NnueUpdates&);

    template <NnueUpdateAction kNnueAction>
    Position Position::applyMove(Move move, eval::NnueState* nnueState) const {
        static constexpr bool kUpdateNnue = kNnueAction != NnueUpdateAction::kNone;

        if constexpr (kUpdateNnue) {
            assert(nnueState != nullptr);
        }

        auto newPos = *this;

        newPos.m_stm = oppColor(m_stm);
        newPos.m_keys.flipStm();

        const auto stm = newPos.nstm();
        const auto nstm = oppColor(stm);

        if (stm == Color::kBlack) {
            ++newPos.m_fullmove;
        }

        if (!move) {
            newPos.m_pinned = newPos.calcPinned();
            newPos.m_threats = newPos.calcThreats();

            return newPos;
        }

        const auto moveSrc = move.fromSq();
        const auto moveDst = move.toSq();

        const auto moving = m_boards.pieceOn(moveSrc);

        eval::NnueUpdates updates{};
        auto captured = Piece::kNone;

        if (move.isPromo()) {
            captured = newPos.promotePawn<true, kUpdateNnue>(moving, moveSrc, moveDst, updates);
        } else {
            captured = newPos.movePiece<true, kUpdateNnue>(moving, moveSrc, moveDst, updates);
        }

        assert(pieceTypeOrNone(captured) != PieceType::kKing);

        if constexpr (kUpdateNnue) {
            nnueState->pushUpdates<kNnueAction == NnueUpdateAction::kApply>(updates, m_boards.bbs(), m_kings);
        }

        if (captured == Piece::kNone && pieceType(moving) != PieceType::kPawn) {
            ++newPos.m_halfmove;
        } else {
            newPos.m_halfmove = 0;
        }

        newPos.m_checkers = newPos.calcCheckers();
        newPos.m_pinned = newPos.calcPinned();
        newPos.m_threats = newPos.calcThreats();

        return newPos;
    }

    bool Position::isPseudolegal(Move move) const {
        assert(move != kNullMove);

        const auto us = stm();

        const auto src = move.fromSq();
        const auto srcPiece = m_boards.pieceOn(src);

        if (srcPiece == Piece::kNone || pieceColor(srcPiece) != us) {
            return false;
        }

        const auto dst = move.toSq();
        const auto dstPiece = m_boards.pieceOn(dst);

        // we're capturing our own piece or trying to capture a king
        if (dstPiece != Piece::kNone && (pieceColor(dstPiece) == us || pieceType(dstPiece) == PieceType::kKing)) {
            return false;
        }

        const auto srcPieceType = pieceType(srcPiece);
        const auto them = oppColor(us);
        const auto occ = m_boards.bbs().occupancy();

        if (srcPieceType == PieceType::kPawn) {
            const auto srcRank = move.fromSqRank();
            const auto dstRank = move.toSqRank();

            // backwards move
            if ((us == Color::kBlack && dstRank >= srcRank) || (us == Color::kWhite && dstRank <= srcRank)) {
                return false;
            }

            const auto promoRank = relativeRank(us, 7);

            // non-promotion move to back rank, or promotion move to any other rank
            if (move.isPromo() != (dstRank == promoRank)) {
                return false;
            }

            // sideways move
            if (move.fromSqFile() != move.toSqFile()) {
                // not valid attack
                if (!(attacks::getPawnAttacks(src, us) & m_boards.bbs().forColor(them))[dst]) {
                    return false;
                }
            } else if (dstPiece != Piece::kNone) {
                // forward move onto a piece
                return false;
            }

            if (std::abs(dstRank - srcRank) > 1) {
                return false;
            }
        } else {
            if (move.isPromo()) {
                return false;
            }

            Bitboard attacks{};

            switch (srcPieceType) {
                case PieceType::kAlfil:
                    attacks = attacks::getAlfilAttacks(src);
                    break;
                case PieceType::kFerz:
                    attacks = attacks::getFerzAttacks(src);
                    break;
                case PieceType::kKnight:
                    attacks = attacks::getKnightAttacks(src);
                    break;
                case PieceType::kRook:
                    attacks = attacks::getRookAttacks(src, occ);
                    break;
                case PieceType::kKing:
                    attacks = attacks::getKingAttacks(src);
                    break;
                default:
                    __builtin_unreachable();
            }

            if (!attacks[dst]) {
                return false;
            }
        }

        return true;
    }

    // This does *not* check for pseudolegality, moves are assumed to be pseudolegal
    bool Position::isLegal(Move move) const {
        assert(move != kNullMove);

        const auto us = stm();
        const auto them = oppColor(us);

        const auto& bbs = m_boards.bbs();

        const auto src = move.fromSq();
        const auto dst = move.toSq();

        const auto king = m_kings.color(us);

        const auto moving = m_boards.pieceOn(src);

        if (pieceType(moving) == PieceType::kKing) {
            const auto kinglessOcc = bbs.occupancy() ^ bbs.kings(us);
            return !m_threats[move.toSq()] && (attacks::getRookAttacks(dst, kinglessOcc) & bbs.rooks(them)).empty();
        }

        // multiple checks can only be evaded with a king move
        if (m_checkers.multiple() || pinned(us)[src] && !orthoRayIntersecting(src, dst)[king]) {
            return false;
        }

        if (m_checkers.empty()) {
            return true;
        }

        const auto checker = m_checkers.lowestSquare();
        return (orthoRayBetween(king, checker) | Bitboard::fromSquare(checker))[dst];
    }

    // see comment in cuckoo.cpp
    bool Position::hasCycle(i32 ply, std::span<const u64> keys) const {
        const auto end = std::min<i32>(m_halfmove, static_cast<i32>(keys.size()));

        if (end < 3) {
            return false;
        }

        const auto S = [&](i32 d) { return keys[keys.size() - d]; };

        const auto occ = m_boards.bbs().occupancy();
        const auto originalKey = m_keys.all;

        auto other = ~(originalKey ^ S(1));

        for (i32 d = 3; d <= end; d += 2) {
            const auto currKey = S(d);

            other ^= ~(currKey ^ S(d - 1));
            if (other != 0) {
                continue;
            }

            const auto diff = originalKey ^ currKey;

            u32 slot = cuckoo::h1(diff);

            if (diff != cuckoo::keys[slot]) {
                slot = cuckoo::h2(diff);
            }

            if (diff != cuckoo::keys[slot]) {
                continue;
            }

            const auto move = cuckoo::moves[slot];

            if ((occ & orthoRayBetween(move.fromSq(), move.toSq())).empty()) {
                // repetition is after root, done
                if (ply > d) {
                    return true;
                }

                auto piece = m_boards.pieceOn(move.fromSq());
                if (piece == Piece::kNone) {
                    piece = m_boards.pieceOn(move.toSq());
                }

                assert(piece != Piece::kNone);

                return pieceColor(piece) == stm();
            }
        }

        return false;
    }

    bool Position::isDrawn(i32 ply, std::span<const u64> keys) const {
        const auto halfmove = m_halfmove;

        if (halfmove >= 140) {
            if (!isCheck()) {
                return true;
            }

            //TODO there's a speedup possible here, but
            // it requires a lot of movegen refactoring
            ScoredMoveList moves{};
            generateAll(moves, *this);

            return std::ranges::any_of(moves, [this](const auto move) { return isLegal(move.move); });
        }

        const auto currKey = m_keys.all;
        const auto limit = std::max(0, static_cast<i32>(keys.size()) - halfmove - 2);

        ply -= 4;

        i32 repetitions = 0;

        for (auto i = static_cast<i32>(keys.size()) - 4; i >= limit; i -= 2, ply -= 2) {
            // require a threefold repetition before root
            if (keys[i] == currKey && ++repetitions == 1 + (ply < 0)) {
                return true;
            }
        }

        const auto& bbs = this->bbs();

        // KK
        if (bbs.occupancy() == bbs.kings()) {
            return true;
        }

        //TODO more?

        return false;
    }

    std::string Position::toFen() const {
        std::string fen{};
        auto itr = std::back_inserter(fen);

        for (i32 rank = 7; rank >= 0; --rank) {
            for (i32 file = 0; file < 8; ++file) {
                if (m_boards.pieceAt(rank, file) == Piece::kNone) {
                    u32 emptySquares = 1;
                    for (; file < 7 && m_boards.pieceAt(rank, file + 1) == Piece::kNone; ++file, ++emptySquares) {
                    }

                    fmt::format_to(itr, "{}", static_cast<char>('0' + emptySquares));
                } else {
                    fmt::format_to(itr, "{}", m_boards.pieceAt(rank, file));
                }
            }

            if (rank > 0) {
                fmt::format_to(itr, "/");
            }
        }

        fmt::format_to(itr, "{}", stm() == Color::kWhite ? " w " : " b ");

        fmt::format_to(itr, " - -");

        fmt::format_to(itr, " {} {}", m_halfmove, m_fullmove);

        return fen;
    }

    template <bool kUpdateKey>
    void Position::setPiece(Piece piece, Square square) {
        assert(piece != Piece::kNone);
        assert(square != Square::kNone);

        assert(pieceType(piece) != PieceType::kKing);

        m_boards.setPiece(square, piece);

        if constexpr (kUpdateKey) {
            m_keys.flipPiece(piece, square);
        }
    }

    template <bool kUpdateKey>
    void Position::removePiece(Piece piece, Square square) {
        assert(piece != Piece::kNone);
        assert(square != Square::kNone);

        assert(pieceType(piece) != PieceType::kKing);

        m_boards.removePiece(square, piece);

        if constexpr (kUpdateKey) {
            m_keys.flipPiece(piece, square);
        }
    }

    template <bool kUpdateKey>
    void Position::movePieceNoCap(Piece piece, Square src, Square dst) {
        assert(piece != Piece::kNone);

        assert(src != Square::kNone);
        assert(dst != Square::kNone);

        if (src == dst) {
            return;
        }

        m_boards.movePiece(src, dst, piece);

        if (pieceType(piece) == PieceType::kKing) {
            const auto color = pieceColor(piece);
            m_kings.color(color) = dst;
        }

        if constexpr (kUpdateKey) {
            m_keys.movePiece(piece, src, dst);
        }
    }

    template <bool kUpdateKey, bool kUpdateNnue>
    Piece Position::movePiece(Piece piece, Square src, Square dst, eval::NnueUpdates& nnueUpdates) {
        assert(piece != Piece::kNone);

        assert(src != Square::kNone);
        assert(dst != Square::kNone);
        assert(src != dst);

        const auto captured = m_boards.pieceOn(dst);

        if (captured != Piece::kNone) {
            assert(pieceType(captured) != PieceType::kKing);

            m_boards.removePiece(dst, captured);

            // NNUE update done below

            if constexpr (kUpdateKey) {
                m_keys.flipPiece(captured, dst);
            }
        }

        m_boards.movePiece(src, dst, piece);

        if (pieceType(piece) == PieceType::kKing) {
            const auto color = pieceColor(piece);

            if constexpr (kUpdateNnue) {
                if (eval::InputFeatureSet::refreshRequired(color, m_kings.color(color), dst)) {
                    nnueUpdates.setRefresh(color);
                }
            }

            m_kings.color(color) = dst;
        }

        if constexpr (kUpdateNnue) {
            nnueUpdates.pushSubAdd(piece, src, dst);

            if (captured != Piece::kNone) {
                nnueUpdates.pushSub(captured, dst);
            }
        }

        if constexpr (kUpdateKey) {
            m_keys.movePiece(piece, src, dst);
        }

        return captured;
    }

    template <bool kUpdateKey, bool kUpdateNnue>
    Piece Position::promotePawn(Piece pawn, Square src, Square dst, eval::NnueUpdates& nnueUpdates) {
        assert(pawn != Piece::kNone);
        assert(pieceType(pawn) == PieceType::kPawn);

        assert(src != Square::kNone);
        assert(dst != Square::kNone);
        assert(src != dst);

        assert(squareRank(dst) == relativeRank(pieceColor(pawn), 7));
        assert(squareRank(src) == relativeRank(pieceColor(pawn), 6));

        const auto captured = m_boards.pieceOn(dst);

        if (captured != Piece::kNone) {
            assert(pieceType(captured) != PieceType::kKing);

            m_boards.removePiece(dst, captured);

            if constexpr (kUpdateNnue) {
                nnueUpdates.pushSub(captured, dst);
            }

            if constexpr (kUpdateKey) {
                m_keys.flipPiece(captured, dst);
            }
        }

        m_boards.moveAndChangePiece(src, dst, pawn, PieceType::kFerz);

        if constexpr (kUpdateNnue || kUpdateKey) {
            const auto coloredFerz = copyPieceColor(pawn, PieceType::kFerz);

            if constexpr (kUpdateNnue) {
                nnueUpdates.pushSub(pawn, src);
                nnueUpdates.pushAdd(coloredFerz, dst);
            }

            if constexpr (kUpdateKey) {
                m_keys.flipPiece(pawn, src);
                m_keys.flipPiece(coloredFerz, dst);
            }
        }

        return captured;
    }

    void Position::regen() {
        m_boards.regenFromBbs();

        m_keys.clear();

        for (u32 rank = 0; rank < 8; ++rank) {
            for (u32 file = 0; file < 8; ++file) {
                const auto square = toSquare(rank, file);
                if (const auto piece = m_boards.pieceOn(square); piece != Piece::kNone) {
                    if (pieceType(piece) == PieceType::kKing) {
                        m_kings.color(pieceColor(piece)) = square;
                    }

                    m_keys.flipPiece(piece, toSquare(rank, file));
                }
            }
        }

        if (stm() == Color::kBlack) {
            m_keys.flipStm();
        }

        m_checkers = calcCheckers();
        m_pinned = calcPinned();
        m_threats = calcThreats();
    }

    Move Position::moveFromUci(std::string_view move) const {
        if (move.length() < 4 || move.length() > 5) {
            return kNullMove;
        }

        if (move.length() == 5 && move[4] != 'q') {
            return kNullMove;
        }

        const auto src = squareFromString(move.substr(0, 2));
        const auto dst = squareFromString(move.substr(2, 2));

        const auto moving = pieceType(boards().pieceOn(src));
        const auto promoRank = relativeRank(stm(), 7);

        return (moving == PieceType::kPawn && squareRank(dst) == promoRank) ? Move::promotion(src, dst)
                                                                            : Move::standard(src, dst);
    }

    Position Position::starting() {
        Position pos{};

        auto& bbs = pos.m_boards.bbs();

        bbs.forPiece(PieceType::kPawn) = U64(0x00FF00000000FF00);
        bbs.forPiece(PieceType::kAlfil) = U64(0x2400000000000024);
        bbs.forPiece(PieceType::kFerz) = U64(0x1000000000000010);
        bbs.forPiece(PieceType::kKnight) = U64(0x4200000000000042);
        bbs.forPiece(PieceType::kRook) = U64(0x8100000000000081);
        bbs.forPiece(PieceType::kKing) = U64(0x0800000000000008);

        bbs.forColor(Color::kBlack) = U64(0xFFFF000000000000);
        bbs.forColor(Color::kWhite) = U64(0x000000000000FFFF);

        pos.m_stm = Color::kWhite;
        pos.m_fullmove = 1;

        pos.regen();

        return pos;
    }

    std::optional<Position> Position::fromFenParts(std::span<const std::string_view> fen) {
        if (fen.size() < 4 || fen.size() > 6) {
            eprintln("wrong number of FEN parts");
            return {};
        }

        Position pos{};
        const auto& bbs = pos.bbs();

        u32 rankIdx = 0;

        std::vector<std::string_view> ranks{};
        split::split(ranks, fen[0], '/');

        for (const auto rank : ranks) {
            if (rankIdx >= 8) {
                eprintln("too many ranks");
                return {};
            }

            u32 fileIdx = 0;

            for (const auto c : rank) {
                if (fileIdx >= 8) {
                    eprintln("too many files in rank {}", rankIdx);
                    return {};
                }

                if (const auto emptySquares = util::tryParseDigit(c)) {
                    fileIdx += *emptySquares;
                } else if (const auto piece = pieceFromChar(c); piece != Piece::kNone) {
                    pos.m_boards.setPiece(toSquare(7 - rankIdx, fileIdx), piece);
                    ++fileIdx;
                } else {
                    eprintln("invalid piece character {}", c);
                    return {};
                }
            }

            // last character was a digit
            if (fileIdx > 8) {
                eprintln("too many files in rank {}", rankIdx);
                return {};
            }

            if (fileIdx < 8) {
                eprintln("not enough files in rank {}", rankIdx);
                return {};
            }

            ++rankIdx;
        }

        if (const auto blackKingCount = bbs.forPiece(Piece::kBlackKing).popcount(); blackKingCount != 1) {
            eprintln("black must have exactly 1 king, but has {}", blackKingCount);
            return {};
        }

        if (const auto whiteKingCount = bbs.forPiece(Piece::kWhiteKing).popcount(); whiteKingCount != 1) {
            eprintln("white must have exactly 1 king, but has {}", whiteKingCount);
            return {};
        }

        if (bbs.occupancy().popcount() > 32) {
            eprintln("too many pieces");
            return {};
        }

        const auto color = fen[1];

        if (color.length() != 1) {
            eprintln("invalid side to move");
            return {};
        }

        switch (color[0]) {
            case 'b':
                pos.m_stm = Color::kBlack;
                break;
            case 'w':
                pos.m_stm = Color::kWhite;
                break;
            default:
                eprintln("invalid side to move");
                return {};
        }

        if (const auto stm = pos.stm();
            pos.isAttacked<false>(stm, bbs.forPiece(PieceType::kKing, oppColor(stm)).lowestSquare(), stm))
        {
            eprintln("opponent must not be in check");
            return {};
        }

        if (fen[2] != "-") {
            eprintln("invalid 3rd field");
            return {};
        }

        if (fen[3] != "-") {
            eprintln("invalid 4th field");
            return {};
        }

        if (fen.size() >= 5) {
            const auto halfmove = fen[4];
            if (!util::tryParse(pos.m_halfmove, halfmove)) {
                eprintln("invalid halfmove clock");
                return {};
            }
        }

        if (fen.size() >= 6) {
            const auto fullmove = fen[5];
            if (!util::tryParse(pos.m_fullmove, fullmove)) {
                eprintln("invalid fullmove number");
                return {};
            }
        }

        pos.regen();

        return pos;
    }

    std::optional<Position> Position::fromFen(std::string_view fen) {
        std::vector<std::string_view> parts{};
        parts.reserve(6);

        split::split(parts, fen, ' ');

        return fromFenParts(parts);
    }

    Square squareFromString(std::string_view str) {
        if (str.length() != 2) {
            return Square::kNone;
        }

        const auto file = str[0];
        const auto rank = str[1];

        if (file < 'a' || file > 'h' || rank < '1' || rank > '8') {
            return Square::kNone;
        }

        return toSquare(static_cast<u32>(rank - '1'), static_cast<u32>(file - 'a'));
    }
} // namespace oranj

fmt::format_context::iterator fmt::formatter<oranj::Position>::format(const oranj::Position& value, format_context& ctx)
    const {
    using namespace oranj;

    const auto& boards = value.boards();

    for (i32 rank = 7; rank >= 0; --rank) {
        format_to(ctx.out(), " +---+---+---+---+---+---+---+---+\n");

        for (i32 file = 0; file < 8; ++file) {
            const auto piece = boards.pieceAt(rank, file);
            format_to(ctx.out(), " | {}", piece);
        }

        format_to(ctx.out(), " | {}\n", rank + 1);
    }

    format_to(ctx.out(), " +---+---+---+---+---+---+---+---+\n");
    format_to(ctx.out(), "   a   b   c   d   e   f   g   h\n");

    format_to(ctx.out(), "\n");

    format_to(ctx.out(), "{} to move", value.stm() == Color::kBlack ? "Black" : "White");

    return ctx.out();
}
