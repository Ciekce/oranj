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

#include "position.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iterator>
#include <utility>
#include <vector>

#include "attacks/attacks.h"
#include "cuckoo.h"
#include "eval/nnue_state.h"
#include "movegen.h"
#include "opts.h"
#include "rays.h"
#include "util/parse.h"
#include "util/split.h"

namespace oranj {
    using NnueObserver = eval::BoardObserver;

    template <typename Observer>
    Position Position::applyMove(Move move, Observer observer) const {
        auto newPos = *this;

        newPos.m_stm = m_stm.flip();
        newPos.m_keys.flipStm();

        if (m_stm == Colors::kBlack) {
            ++newPos.m_fullmove;
        }

        if (!move) {
            newPos.calcCheckersAndPins();
            newPos.calcThreats();

            newPos.calcCheckZones();

            return newPos;
        }

        const auto moveSrc = move.fromSq();
        const auto moveDst = move.toSq();

        const auto moving = pieceOn(moveSrc);

        Piece captured;

        if (move.isPromo()) {
            captured = newPos.promotePawn<true, Observer>(moving, moveSrc, moveDst, observer);
        } else {
            captured = newPos.movePiece<true, Observer>(moving, moveSrc, moveDst, observer);
        }

        assert(captured.typeOrNone() != PieceTypes::kKing);

        observer.finalize(*this, newPos);

        if (captured == Pieces::kNone && moving.type() != PieceTypes::kPawn) {
            ++newPos.m_halfmove;
        } else {
            newPos.m_halfmove = 0;
        }

        newPos.calcCheckersAndPins();
        newPos.calcThreats();

        newPos.calcCheckZones();

        return newPos;
    }

    template Position Position::applyMove<NullObserver>(Move, NullObserver) const;
    template Position Position::applyMove<NnueObserver>(Move, NnueObserver) const;

    bool Position::isLegal(Move move) const {
        assert(move != kNullMove);

        const auto us = stm();

        const auto kingSq = m_kings.color(us);

        const auto src = move.fromSq();
        const auto dst = move.toSq();

        if (src == dst) {
            return false;
        }

        const auto srcPiece = pieceOn(src);
        const auto dstPiece = pieceOn(dst);

        if (srcPiece == Pieces::kNone || srcPiece.color() != us) {
            return false;
        }

        if (m_checkers && srcPiece.type() != PieceTypes::kKing) {
            // multiple checks can only be evaded with a king move
            if (m_checkers.multiple()) {
                return false;
            }

            // one checker may be evaded, blocked, or captured
            const auto checker = m_checkers.lowestSquare();
            if (!(rayBetween(kingSq, checker) | Bitboard::fromSquare(checker)).hasSq(dst)) {
                return false;
            }
        }

        // pinned pieces can only move along their pin ray
        if (pinned(us).hasSq(src) && !rayIntersecting(src, dst).hasSq(kingSq)) {
            return false;
        }

        if (dstPiece != Pieces::kNone && dstPiece.color() == us) {
            return false;
        }

        const auto srcPieceType = srcPiece.type();
        const auto them = us.flip();
        const auto occ = this->occ();

        if (srcPieceType == PieceTypes::kPawn) {
            const auto srcRank = src.rank();
            const auto dstRank = dst.rank();

            // backwards move
            if ((us == Colors::kBlack && dstRank >= srcRank) || (us == Colors::kWhite && dstRank <= srcRank)) {
                return false;
            }

            const auto promoRank = relativeRank(us, 7);

            // non-promotion move to back rank, or promotion move to any other rank
            if (move.isPromo() != (dstRank == promoRank)) {
                return false;
            }

            // sideways move
            if (src.file() != dst.file()) {
                // not valid attack
                if (!(attacks::getPawnAttacks(src, us) & bb(them)).hasSq(dst)) {
                    return false;
                }
            } else if (dstPiece != Pieces::kNone) {
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

            Bitboard attacks;

            switch (srcPieceType.raw()) {
                case PieceTypes::kAlfil.raw():
                    attacks = attacks::getAlfilAttacks(src);
                    break;
                case PieceTypes::kFerz.raw():
                    attacks = attacks::getFerzAttacks(src);
                    break;
                case PieceTypes::kKnight.raw():
                    attacks = attacks::getKnightAttacks(src);
                    break;
                case PieceTypes::kRook.raw():
                    attacks = attacks::getRookAttacks(src, occ);
                    break;
                case PieceTypes::kKing.raw():
                    attacks = attacks::getKingAttacks(src) & ~m_threats;
                    break;
                default:
                    __builtin_unreachable();
            }

            if (!attacks.hasSq(dst)) {
                return false;
            }
        }

        return true;
    }

    u64 Position::roughKeyAfter(Move move) const {
        assert(move);

        const auto moving = pieceOn(move.fromSq());
        assert(moving != Pieces::kNone);

        const auto captured = pieceOn(move.toSq());

        auto key = m_keys.all;

        key ^= keys::pieceSquare(moving, move.fromSq());
        key ^= keys::pieceSquare(moving, move.toSq());

        if (captured != Pieces::kNone) {
            key ^= keys::pieceSquare(captured, move.toSq());
        }

        key ^= keys::color();

        return key;
    }

    Bitboard Position::allAttackersTo(Square sq, Bitboard occ) const {
        assert(sq != Squares::kNone);

        const auto& bbs = this->bbs();

        Bitboard attackers{};

        const auto rooks = bbs.rooks();
        attackers |= rooks & attacks::getRookAttacks(sq, occ);

        attackers |= bbs.blackPawns() & attacks::getPawnAttacks(sq, Colors::kWhite);
        attackers |= bbs.whitePawns() & attacks::getPawnAttacks(sq, Colors::kBlack);

        const auto alfils = bbs.alfils();
        attackers |= alfils & attacks::getAlfilAttacks(sq);

        const auto ferzes = bbs.ferzes();
        attackers |= ferzes & attacks::getFerzAttacks(sq);

        const auto knights = bbs.knights();
        attackers |= knights & attacks::getKnightAttacks(sq);

        const auto kings = bbs.kings();
        attackers |= kings & attacks::getKingAttacks(sq);

        return attackers;
    }

    Bitboard Position::nonSliderAttackersTo(Square sq, Color attacker) const {
        assert(sq != Squares::kNone);

        const auto& bbs = this->bbs();

        Bitboard attackers{};

        const auto pawns = bbs.pawns(attacker);
        attackers |= pawns & attacks::getPawnAttacks(sq, attacker.flip());

        const auto alfils = bbs.alfils(attacker);
        attackers |= alfils & attacks::getAlfilAttacks(sq);

        const auto ferzes = bbs.ferzes(attacker);
        attackers |= ferzes & attacks::getFerzAttacks(sq);

        const auto knights = bbs.knights(attacker);
        attackers |= knights & attacks::getKnightAttacks(sq);

        const auto kings = bbs.kings(attacker);
        attackers |= kings & attacks::getKingAttacks(sq);

        return attackers;
    }

    Bitboard Position::attackersTo(Square sq, Color attacker) const {
        assert(sq != Squares::kNone);

        auto attackers = nonSliderAttackersTo(sq, attacker);

        const auto& bbs = this->bbs();

        const auto occ = this->occ();

        const auto rooks = bbs.rooks(attacker);
        attackers |= rooks & attacks::getRookAttacks(sq, occ);

        return attackers;
    }

    template bool Position::isAttacked<false>(Color toMove, Square sq, Color attacker) const;
    template bool Position::isAttacked<true>(Color toMove, Square sq, Color attacker) const;

    template <bool kThreatShortcut>
    bool Position::isAttacked(Color toMove, Square sq, Color attacker) const {
        assert(toMove != Colors::kNone);
        assert(sq != Squares::kNone);
        assert(attacker != Colors::kNone);

        if constexpr (kThreatShortcut) {
            if (attacker != toMove) {
                return m_threats.hasSq(sq);
            }
        }

        if (const auto alfils = m_bbs.alfils(attacker); !(alfils & attacks::getAlfilAttacks(sq)).empty()) {
            return true;
        }

        if (const auto ferzes = m_bbs.ferzes(attacker); !(ferzes & attacks::getFerzAttacks(sq)).empty()) {
            return true;
        }

        if (const auto knights = m_bbs.knights(attacker); !(knights & attacks::getKnightAttacks(sq)).empty()) {
            return true;
        }

        if (const auto pawns = m_bbs.pawns(attacker); !(pawns & attacks::getPawnAttacks(sq, attacker.flip())).empty()) {
            return true;
        }

        if (const auto kings = m_bbs.kings(attacker); !(kings & attacks::getKingAttacks(sq)).empty()) {
            return true;
        }

        const auto occ = this->occ();
        if (const auto rooks = m_bbs.rooks(attacker); !(rooks & attacks::getRookAttacks(sq, occ)).empty()) {
            return true;
        }

        return false;
    }

    bool Position::anyAttacked(Bitboard squares, Color attacker) const {
        assert(attacker != Colors::kNone);

        if (attacker == nstm()) {
            return !(squares & m_threats).empty();
        }

        for (const auto sq : squares) {
            if (isAttacked(sq, attacker)) {
                return true;
            }
        }

        return false;
    }

    // see comment in cuckoo.cpp
    bool Position::hasUpcomingRepetition(i32 ply, std::span<const u64> keys) const {
        const auto end = std::min<i32>(m_halfmove, static_cast<i32>(keys.size()));

        if (end < 3) {
            return false;
        }

        const auto prevKey = [&](i32 d) { return keys[keys.size() - d]; };

        const auto occ = this->occ();
        const auto originalKey = m_keys.all;

        auto other = originalKey ^ prevKey(1);

        for (i32 d = 3; d <= end; d += 2) {
            const auto currKey = prevKey(d);

            other ^= currKey ^ prevKey(d - 1);
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

            if ((occ & rayBetween(move.fromSq(), move.toSq())).empty()) {
                // repetition is after root, done
                if (ply > d) {
                    return true;
                }

                // otherwise, require a threefold
                for (i32 i = d + 2; i <= end; ++i) {
                    if (currKey == prevKey(i)) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool Position::isDrawnByRepetition(i32 ply, std::span<const u64> keys) const {
        const auto currKey = m_keys.all;
        const auto limit = std::max(0, static_cast<i32>(keys.size()) - m_halfmove - 2);

        ply -= 4;

        i32 repetitions = 0;

        for (auto i = static_cast<i32>(keys.size()) - 4; i >= limit; i -= 2, ply -= 2) {
            // require a threefold repetition before root
            if (keys[i] == currKey && ++repetitions == 1 + (ply < 0)) {
                return true;
            }
        }

        return false;
    }

    bool Position::isDrawn(i32 ply, std::span<const u64> keys) const {
        if (m_halfmove >= 140) {
            if (!isCheck()) {
                return true;
            }

            //TODO there's a speedup possible here, but
            // it requires a lot of movegen refactoring
            ScoredMoveList moves{};
            generateAll(moves, *this);

            return !moves.empty();
        }

        if (isDrawnByRepetition(ply, keys)) {
            return true;
        }

        const auto& bbs = this->bbs();

        // KK
        if (bbs.occ() == bbs.kings()) {
            return true;
        }

        return false;
    }

    Piece Position::captureTarget(Move move) const {
        assert(move != kNullMove);
        return pieceOn(move.toSq());
    }

    bool Position::isNoisy(Move move) const {
        assert(move != kNullMove);
        return pieceOn(move.toSq()) != Pieces::kNone;
    }

    bool Position::givesDirectCheck(Move move) const {
        assert(move != kNullMove);

        const auto movingPt = move.isPromo() ? PieceTypes::kFerz : pieceOn(move.fromSq()).type();

        if (movingPt == PieceTypes::kKing) {
            return false;
        }

        const auto checkZone = m_checkZones[movingPt.idx()];
        return checkZone.hasSq(move.toSq());
    }

    std::string Position::toFen() const {
        std::string fen{};
        auto itr = std::back_inserter(fen);

        for (i32 rank = 7; rank >= 0; --rank) {
            for (i32 file = 0; file < 8; ++file) {
                const auto sq = Square::fromFileRank(file, rank);
                if (pieceOn(sq) == Pieces::kNone) {
                    u32 emptySquares = 1;
                    for (; file < 7 && pieceOn(Square::fromFileRank(file + 1, rank)) == Pieces::kNone;
                         ++file, ++emptySquares)
                    {}
                    fmt::format_to(itr, "{}", static_cast<char>('0' + emptySquares));
                } else {
                    fmt::format_to(itr, "{}", pieceOn(sq));
                }
            }

            if (rank > 0) {
                fmt::format_to(itr, "/");
            }
        }

        fmt::format_to(itr, "{} - - {} {}", stm() == Colors::kWhite ? " w " : " b ", m_halfmove, m_fullmove);

        return fen;
    }

    void Position::regen() {
        m_mailbox.fill(Pieces::kNone);
        m_keys.clear();

        for (u32 pieceIdx = 0; pieceIdx < Pieces::kCount; ++pieceIdx) {
            const auto piece = Piece::fromRaw(pieceIdx);
            for (const auto sq : m_bbs.bb(piece)) {
                assert(mailboxSlot(sq) == Pieces::kNone);

                mailboxSlot(sq) = piece;

                if (piece.type() == PieceTypes::kKing) {
                    m_kings.color(piece.color()) = sq;
                }

                m_keys.flipPiece(piece, sq);
            }
        }

        if (stm() == Colors::kBlack) {
            m_keys.flipStm();
        }

        calcCheckersAndPins();
        calcThreats();

        calcCheckZones();
    }

    Move Position::moveFromUci(std::string_view move) const {
        if (move.length() < 4 || move.length() > 5) {
            return kNullMove;
        }

        const auto src = Square::fromStr(move.substr(0, 2));
        const auto dst = Square::fromStr(move.substr(2, 2));

        if (!src || !dst) {
            return kNullMove;
        }

        if (move.length() == 5) {
            if (move[4] != 'q') {
                return kNullMove;
            }
            return Move::promotion(src, dst);
        } else {
            return Move::standard(src, dst);
        }
    }

    Position Position::startpos() {
        Position pos{};

        pos.m_bbs.bb(PieceTypes::kPawn) = U64(0x00FF00000000FF00);
        pos.m_bbs.bb(PieceTypes::kAlfil) = U64(0x2400000000000024);
        pos.m_bbs.bb(PieceTypes::kFerz) = U64(0x1000000000000010);
        pos.m_bbs.bb(PieceTypes::kKnight) = U64(0x4200000000000042);
        pos.m_bbs.bb(PieceTypes::kRook) = U64(0x8100000000000081);
        pos.m_bbs.bb(PieceTypes::kKing) = U64(0x0800000000000008);

        pos.m_bbs.bb(Colors::kBlack) = U64(0xFFFF000000000000);
        pos.m_bbs.bb(Colors::kWhite) = U64(0x000000000000FFFF);

        pos.m_stm = Colors::kWhite;
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

        i32 rankIdx = 0;

        std::vector<std::string_view> ranks{};
        split::split(ranks, fen[0], '/');

        for (const auto rank : ranks) {
            if (rankIdx >= 8) {
                eprintln("too many ranks");
                return {};
            }

            i32 fileIdx = 0;

            for (const auto c : rank) {
                if (fileIdx >= 8) {
                    eprintln("too many files in rank {}", rankIdx);
                    return {};
                }

                if (const auto emptySquares = util::tryParseDigit(c)) {
                    fileIdx += *emptySquares;
                } else if (const auto piece = Piece::fromChar(c); piece != Pieces::kNone) {
                    pos.setPieceInternal(Square::fromFileRank(fileIdx, 7 - rankIdx), piece);
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

        if (const auto blackKingCount = pos.bb(Pieces::kBlackKing).popcount(); blackKingCount != 1) {
            eprintln("black must have exactly 1 king, but has {}", blackKingCount);
            return {};
        }

        if (const auto whiteKingCount = pos.bb(Pieces::kWhiteKing).popcount(); whiteKingCount != 1) {
            eprintln("white must have exactly 1 king, but has {}", whiteKingCount);
            return {};
        }

        if (pos.occ().popcount() > 32) {
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
                pos.m_stm = Colors::kBlack;
                break;
            case 'w':
                pos.m_stm = Colors::kWhite;
                break;
            default:
                eprintln("invalid side to move");
                return {};
        }

        if (const auto stm = pos.stm();
            pos.isAttacked<false>(stm, bbs.bb(PieceTypes::kKing, stm.flip()).lowestSquare(), stm))
        {
            eprintln("opponent must not be in check");
            return {};
        }

        if (fen[2] != "-") {
            eprintln("invalid third field");
            return {};
        }

        if (fen[3] != "-") {
            eprintln("invalid fourth field");
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

    template <bool kUpdateKey>
    void Position::setPiece(Piece piece, Square sq) {
        assert(piece != Pieces::kNone);
        assert(sq != Squares::kNone);

        assert(piece.type() != PieceTypes::kKing);

        setPieceInternal(sq, piece);

        if constexpr (kUpdateKey) {
            m_keys.flipPiece(piece, sq);
        }
    }

    template void Position::setPiece<false>(Piece, Square);
    template void Position::setPiece<true>(Piece, Square);

    template <bool kUpdateKey>
    void Position::removePiece(Piece piece, Square sq) {
        assert(piece != Pieces::kNone);
        assert(sq != Squares::kNone);

        assert(piece.type() != PieceTypes::kKing);

        removePieceInternal(sq, piece);

        if constexpr (kUpdateKey) {
            m_keys.flipPiece(piece, sq);
        }
    }

    template void Position::removePiece<false>(Piece, Square);
    template void Position::removePiece<true>(Piece, Square);

    template <bool kUpdateKey, typename Observer>
    Piece Position::movePiece(Piece piece, Square src, Square dst, Observer observer) {
        assert(piece != Pieces::kNone);

        assert(src != Squares::kNone);
        assert(dst != Squares::kNone);
        assert(src != dst);

        if (piece.type() == PieceTypes::kKing) {
            const auto color = piece.color();
            observer.prepareKingMove(color, m_kings.color(color), dst);
            m_kings.color(color) = dst;
        }

        const auto captured = pieceOn(dst);

        if (captured != Pieces::kNone) {
            removePieceInternal(src, piece);
            observer.pieceRemoved(*this, piece, src);
            removePieceInternal(dst, captured);
            setPieceInternal(dst, piece);
            observer.pieceMutated(*this, captured, piece, dst);
            if constexpr (kUpdateKey) {
                m_keys.flipPiece(captured, dst);
            }
        } else {
            movePieceInternal(src, dst, piece);
            observer.pieceMoved(*this, piece, src, dst);
        }

        if constexpr (kUpdateKey) {
            m_keys.movePiece(piece, src, dst);
        }

        return captured;
    }

    template Piece Position::movePiece<false, NullObserver>(Piece, Square, Square, NullObserver);
    template Piece Position::movePiece<true, NullObserver>(Piece, Square, Square, NullObserver);
    template Piece Position::movePiece<false, NnueObserver>(Piece, Square, Square, NnueObserver);
    template Piece Position::movePiece<true, NnueObserver>(Piece, Square, Square, NnueObserver);

    template <bool kUpdateKey, typename Observer>
    Piece Position::promotePawn(Piece pawn, Square src, Square dst, Observer observer) {
        assert(pawn != Pieces::kNone);
        assert(pawn.type() == PieceTypes::kPawn);

        assert(src != Squares::kNone);
        assert(dst != Squares::kNone);
        assert(src != dst);

        assert(dst.rank() == relativeRank(pawn.color(), 7));
        assert(src.rank() == relativeRank(pawn.color(), 6));

        const auto captured = pieceOn(dst);
        const auto ferz = pawn.copyColor(PieceTypes::kFerz);

        if (captured != Pieces::kNone) {
            removePieceInternal(src, pawn);
            observer.pieceRemoved(*this, pawn, src);
            removePieceInternal(dst, captured);
            setPieceInternal(dst, ferz);
            observer.pieceMutated(*this, captured, ferz, dst);
            if constexpr (kUpdateKey) {
                m_keys.flipPiece(captured, dst);
            }
        } else {
            moveAndChangePieceInternal(src, dst, pawn, PieceTypes::kFerz);
            observer.piecePromoted(*this, pawn, src, ferz, dst);
        }

        if constexpr (kUpdateKey) {
            m_keys.flipPiece(pawn, src);
            m_keys.flipPiece(ferz, dst);
        }

        return captured;
    }

    template Piece Position::promotePawn<false, NullObserver>(Piece, Square, Square, NullObserver);
    template Piece Position::promotePawn<true, NullObserver>(Piece, Square, Square, NullObserver);
    template Piece Position::promotePawn<false, NnueObserver>(Piece, Square, Square, NnueObserver);
    template Piece Position::promotePawn<true, NnueObserver>(Piece, Square, Square, NnueObserver);

    void Position::setPieceInternal(Square sq, Piece piece) {
        assert(sq != Squares::kNone);
        assert(piece != Pieces::kNone);

        assert(pieceOn(sq) == Pieces::kNone);

        mailboxSlot(sq) = piece;

        const auto mask = Bitboard::fromSquare(sq);

        m_bbs.bb(piece.type()) ^= mask;
        m_bbs.bb(piece.color()) ^= mask;
    }

    void Position::movePieceInternal(Square src, Square dst, Piece piece) {
        assert(src != Squares::kNone);
        assert(dst != Squares::kNone);

        if (mailboxSlot(src) == piece) [[likely]] {
            mailboxSlot(src) = Pieces::kNone;
        }

        mailboxSlot(dst) = piece;

        const auto mask = Bitboard::fromSquare(src) ^ Bitboard::fromSquare(dst);

        m_bbs.bb(piece.type()) ^= mask;
        m_bbs.bb(piece.color()) ^= mask;
    }

    void Position::moveAndChangePieceInternal(Square src, Square dst, Piece moving, PieceType promo) {
        assert(src != Squares::kNone);
        assert(dst != Squares::kNone);
        assert(src != dst);

        assert(moving != Pieces::kNone);
        assert(promo != PieceTypes::kNone);

        assert(pieceOn(src) == moving);
        assert(mailboxSlot(src) == moving);

        mailboxSlot(src) = Pieces::kNone;
        mailboxSlot(dst) = moving.copyColor(promo);

        m_bbs.bb(moving.type()).clearSq(src);
        m_bbs.bb(promo).setSq(dst);

        const auto mask = Bitboard::fromSquare(src) ^ Bitboard::fromSquare(dst);
        m_bbs.bb(moving.color()) ^= mask;
    }

    void Position::removePieceInternal(Square sq, Piece piece) {
        assert(sq != Squares::kNone);
        assert(piece != Pieces::kNone);

        assert(pieceOn(sq) == piece);

        mailboxSlot(sq) = Pieces::kNone;

        m_bbs.bb(piece.type()).clearSq(sq);
        m_bbs.bb(piece.color()).clearSq(sq);
    }

    void Position::calcCheckersAndPins() {
        m_checkers = nonSliderAttackersTo(m_kings.color(m_stm), m_stm.flip());
        m_pinned = {};

        for (const auto c : {Colors::kBlack, Colors::kWhite}) {
            auto& pinned = m_pinned[c.idx()];

            const auto king = m_kings.color(c);
            const auto opponent = c.flip();

            const auto ourOcc = bb(c);
            const auto oppOcc = bb(opponent);

            const auto potentialAttackers = attacks::getRookAttacks(king, oppOcc) & m_bbs.rooks(opponent);

            for (const auto potentialAttacker : potentialAttackers) {
                const auto maybePinned = ourOcc & rayBetween(potentialAttacker, king);
                if (maybePinned.empty()) {
                    assert(c == m_stm);
                    m_checkers.setSq(potentialAttacker);
                } else if (maybePinned.one()) {
                    pinned |= maybePinned;
                }
            }
        }
    }

    void Position::calcThreats() {
        const auto us = stm();
        const auto them = us.flip();

        m_threats = Bitboard{};

        const auto occ = this->occ() & ~m_bbs.kings(us);
        for (const auto rook : m_bbs.rooks(them)) {
            m_threats |= attacks::getRookAttacks(rook, occ);
        }

        for (const auto alfil : m_bbs.alfils(them)) {
            m_threats |= attacks::getAlfilAttacks(alfil);
        }

        for (const auto ferz : m_bbs.ferzes(them)) {
            m_threats |= attacks::getFerzAttacks(ferz);
        }

        for (const auto knight : m_bbs.knights(them)) {
            m_threats |= attacks::getKnightAttacks(knight);
        }

        const auto pawns = m_bbs.pawns(them);
        if (them == Colors::kBlack) {
            m_threats |= pawns.shiftDownLeft() | pawns.shiftDownRight();
        } else {
            m_threats |= pawns.shiftUpLeft() | pawns.shiftUpRight();
        }

        m_threats |= attacks::getKingAttacks(m_kings.color(them));
    }

    void Position::calcCheckZones() {
        const auto oppKingSq = king(nstm());
        const auto occ = this->occ();

        m_checkZones[0] = attacks::getPawnAttacks(oppKingSq, nstm());
        m_checkZones[1] = attacks::getAlfilAttacks(oppKingSq);
        m_checkZones[2] = attacks::getFerzAttacks(oppKingSq);
        m_checkZones[3] = attacks::getKnightAttacks(oppKingSq);
        m_checkZones[4] = attacks::getRookAttacks(oppKingSq, occ);
    }
} // namespace oranj

fmt::format_context::iterator fmt::formatter<oranj::Position>::format(
    const oranj::Position& value,
    format_context& ctx
) const {
    using namespace oranj;

    for (i32 rank = kRank8; rank >= kRank1; --rank) {
        format_to(ctx.out(), " +---+---+---+---+---+---+---+---+\n");

        for (i32 file = kFileA; file <= kFileH; ++file) {
            const auto piece = value.pieceOn(Square::fromFileRank(file, rank));
            format_to(ctx.out(), " | {}", piece);
        }

        format_to(ctx.out(), " | {}\n", rank + 1);
    }

    format_to(ctx.out(), " +---+---+---+---+---+---+---+---+\n");
    format_to(ctx.out(), "   a   b   c   d   e   f   g   h\n");

    format_to(ctx.out(), "\n");

    format_to(ctx.out(), "{} to move", value.stm() == Colors::kBlack ? "Black" : "White");

    return ctx.out();
}
