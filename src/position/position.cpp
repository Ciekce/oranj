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

#ifndef NDEBUG
#include <iostream>
#include <iomanip>
#include "../uci.h"
#include "../pretty.h"
#endif
#include <algorithm>
#include <sstream>
#include <cassert>

#include "../util/parse.h"
#include "../util/split.h"
#include "../attacks/attacks.h"
#include "../movegen.h"
#include "../opts.h"
#include "../rays.h"
#include "../cuckoo.h"

namespace oranj
{
	namespace
	{
		auto scharnaglToBackrank(u32 n)
		{
			// https://en.wikipedia.org/wiki/Fischer_random_chess_numbering_scheme#Direct_derivation

			// these are stored with the second knight moved left by an empty square,
			// because the first knight fills a square before the second knight is placed
			static constexpr auto N5n = std::array{
				std::pair{0, 0},
				std::pair{0, 1},
				std::pair{0, 2},
				std::pair{0, 3},
				std::pair{1, 1},
				std::pair{1, 2},
				std::pair{1, 3},
				std::pair{2, 2},
				std::pair{2, 3},
				std::pair{3, 3}
			};

			assert(n < 960);

			std::array<PieceType, 8> dst{};
			// no need to fill with empty pieces, because pawns are impossible

			const auto placeInNthFree = [&dst](u32 n, PieceType piece)
			{
				for (i32 free = 0, i = 0; i < 8; ++i)
				{
					if (dst[i] == PieceType::Pawn
						&& free++ == n)
					{
						dst[i] = piece;
						break;
					}
				}
			};

			const auto placeInFirstFree = [&dst](PieceType piece)
			{
				for (i32 i = 0; i < 8; ++i)
				{
					if (dst[i] == PieceType::Pawn)
					{
						dst[i] = piece;
						break;
					}
				}
			};

			const auto n2 = n  / 4;
			const auto b1 = n  % 4;

			const auto n3 = n2 / 4;
			const auto b2 = n2 % 4;

			const auto n4 = n3 / 6;
			const auto  q = n3 % 6;

			dst[b1 * 2 + 1] = PieceType::Alfil;
			dst[b2 * 2    ] = PieceType::Alfil;

			placeInNthFree(q, PieceType::Ferz);

			const auto [knight1, knight2] = N5n[n4];

			placeInNthFree(knight1, PieceType::Knight);
			placeInNthFree(knight2, PieceType::Knight);

			placeInFirstFree(PieceType::Rook);
			placeInFirstFree(PieceType::King);
			placeInFirstFree(PieceType::Rook);

			return dst;
		}
	}

	template auto Position::applyMoveUnchecked<false, false>(Move, eval::NnueState *) -> void;
	template auto Position::applyMoveUnchecked<true, false>(Move, eval::NnueState *) -> void;
	template auto Position::applyMoveUnchecked<false, true>(Move, eval::NnueState *) -> void;
	template auto Position::applyMoveUnchecked<true, true>(Move, eval::NnueState *) -> void;

	template auto Position::popMove<false>(eval::NnueState *) -> void;
	template auto Position::popMove<true>(eval::NnueState *) -> void;

	template auto Position::setPiece<false>(Piece, Square) -> void;
	template auto Position::setPiece<true>(Piece, Square) -> void;

	template auto Position::removePiece<false>(Piece, Square) -> void;
	template auto Position::removePiece<true>(Piece, Square) -> void;

	template auto Position::movePieceNoCap<false>(Piece, Square, Square) -> void;
	template auto Position::movePieceNoCap<true>(Piece, Square, Square) -> void;

	template auto Position::movePiece<false, false>(Piece, Square, Square, eval::NnueUpdates &) -> Piece;
	template auto Position::movePiece<true, false>(Piece, Square, Square, eval::NnueUpdates &) -> Piece;
	template auto Position::movePiece<false, true>(Piece, Square, Square, eval::NnueUpdates &) -> Piece;
	template auto Position::movePiece<true, true>(Piece, Square, Square, eval::NnueUpdates &) -> Piece;

	template auto Position::promotePawn<false, false>(Piece, Square, Square, eval::NnueUpdates &) -> Piece;
	template auto Position::promotePawn<true, false>(Piece, Square, Square, eval::NnueUpdates &) -> Piece;
	template auto Position::promotePawn<false, true>(Piece, Square, Square, eval::NnueUpdates &) -> Piece;
	template auto Position::promotePawn<true, true>(Piece, Square, Square, eval::NnueUpdates &) -> Piece;

	Position::Position()
	{
		m_states.reserve(256);
		m_keys.reserve(512);

		m_states.push_back({});
	}

	auto Position::resetToStarting() -> void
	{
		m_states.resize(1);
		m_keys.clear();

		auto &state = currState();
		state = BoardState{};

		auto &bbs = state.boards.bbs();

		bbs.forPiece(PieceType::  Pawn) = U64(0x00FF00000000FF00);
		bbs.forPiece(PieceType:: Alfil) = U64(0x2400000000000024);
		bbs.forPiece(PieceType::  Ferz) = U64(0x1000000000000010);
		bbs.forPiece(PieceType::Knight) = U64(0x4200000000000042);
		bbs.forPiece(PieceType::  Rook) = U64(0x8100000000000081);
		bbs.forPiece(PieceType::  King) = U64(0x0800000000000008);

		bbs.forColor(Color::Black) = U64(0xFFFF000000000000);
		bbs.forColor(Color::White) = U64(0x000000000000FFFF);

		m_blackToMove = false;
		m_fullmove = 1;

		regen();
	}

	auto Position::resetFromFen(const std::string &fen) -> bool
	{
		const auto tokens = split::split(fen, ' ');

		if (tokens.size() > 6)
		{
			std::cerr << "excess tokens after fullmove number in fen " << fen << std::endl;
			return false;
		}

		if (tokens.size() == 5)
		{
			std::cerr << "missing fullmove number in fen " << fen << std::endl;
			return false;
		}

		if (tokens.size() == 4)
		{
			std::cerr << "missing halfmove clock in fen " << fen << std::endl;
			return false;
		}

		if (tokens.size() == 3)
		{
			std::cerr << "missing fourth field in fen " << fen << std::endl;
			return false;
		}

		if (tokens.size() == 2)
		{
			std::cerr << "missing third field in fen " << fen << std::endl;
			return false;
		}

		if (tokens.size() == 1)
		{
			std::cerr << "missing next move color in fen " << fen << std::endl;
			return false;
		}

		if (tokens.empty())
		{
			std::cerr << "missing ranks in fen " << fen << std::endl;
			return false;
		}

		BoardState newState{};
		auto &newBbs = newState.boards.bbs();

		u32 rankIdx = 0;

		const auto ranks = split::split(tokens[0], '/');
		for (const auto &rank : ranks)
		{
			if (rankIdx >= 8)
			{
				std::cerr << "too many ranks in fen " << fen << std::endl;
				return false;
			}

			u32 fileIdx = 0;

			for (const auto c : rank)
			{
				if (fileIdx >= 8)
				{
					std::cerr << "too many files in rank " << rankIdx << " in fen " << fen << std::endl;
					return false;
				}

				if (const auto emptySquares = util::tryParseDigit(c))
					fileIdx += *emptySquares;
				else if (const auto piece = pieceFromChar(c); piece != Piece::None)
				{
					newState.boards.setPiece(toSquare(7 - rankIdx, fileIdx), piece);
					++fileIdx;
				}
				else
				{
					std::cerr << "invalid piece character " << c << " in fen " << fen << std::endl;
					return false;
				}
			}

			// last character was a digit
			if (fileIdx > 8)
			{
				std::cerr << "too many files in rank " << rankIdx << " in fen " << fen << std::endl;
				return false;
			}

			if (fileIdx < 8)
			{
				std::cerr << "not enough files in rank " << rankIdx << " in fen " << fen << std::endl;
				return false;
			}

			++rankIdx;
		}

		if (const auto blackKingCount = newBbs.forPiece(Piece::BlackKing).popcount();
			blackKingCount != 1)
		{
			std::cerr << "black must have exactly 1 king, " << blackKingCount << " in fen " << fen << std::endl;
			return false;
		}

		if (const auto whiteKingCount = newBbs.forPiece(Piece::WhiteKing).popcount();
			whiteKingCount != 1)
		{
			std::cerr << "white must have exactly 1 king, " << whiteKingCount << " in fen " << fen << std::endl;
			return false;
		}

		if (newBbs.occupancy().popcount() > 32)
		{
			std::cerr << "too many pieces in fen " << fen << std::endl;
			return false;
		}

		const auto &color = tokens[1];

		if (color.length() != 1)
		{
			std::cerr << "invalid next move color in fen " << fen << std::endl;
			return false;
		}

		bool newBlackToMove = false;

		switch (color[0])
		{
		case 'b': newBlackToMove = true; break;
		case 'w': break;
		default:
			std::cerr << "invalid next move color in fen " << fen << std::endl;
			return false;
		}

		if (const auto stm = newBlackToMove ? Color::Black : Color::White;
			isAttacked<false>(newState, stm,
				newBbs.forPiece(PieceType::King, oppColor(stm)).lowestSquare(),
				stm))
		{
			std::cerr << "opponent must not be in check" << std::endl;
			return false;
		}

		if (tokens[2] != "-")
		{
			std::cerr << "invalid 3rd field in fen " << fen << std::endl;
			return false;
		}

		if (tokens[3] != "-")
		{
			std::cerr << "invalid 4th field in fen " << fen << std::endl;
			return false;
		}

		const auto &halfmoveStr = tokens[4];

		if (const auto halfmove = util::tryParseU32(halfmoveStr))
			newState.halfmove = *halfmove;
		else
		{
			std::cerr << "invalid halfmove clock in fen " << fen << std::endl;
			return false;
		}

		const auto &fullmoveStr = tokens[5];

		u32 newFullmove;

		if (const auto fullmove = util::tryParseU32(fullmoveStr))
			newFullmove = *fullmove;
		else
		{
			std::cerr << "invalid fullmove number in fen " << fen << std::endl;
			return false;
		}

		m_states.resize(1);
		m_keys.clear();

		m_blackToMove = newBlackToMove;
		m_fullmove = newFullmove;

		currState() = newState;

		regen();

		return true;
	}

	auto Position::resetFromFrcIndex(u32 n) -> bool
	{
		assert(g_opts.chess960);

		if (n >= 960)
		{
			std::cerr << "invalid frc position index " << n << std::endl;
			return false;
		}

		m_states.resize(1);
		m_keys.clear();

		auto &state = currState();
		state = BoardState{};

		auto &bbs = state.boards.bbs();

		bbs.forPiece(PieceType::Pawn) = U64(0x00FF00000000FF00);

		bbs.forColor(Color::Black) = U64(0x00FF000000000000);
		bbs.forColor(Color::White) = U64(0x000000000000FF00);

		const auto backrank = scharnaglToBackrank(n);

		for (i32 i = 0; i < 8; ++i)
		{
			const auto blackSquare = toSquare(7, i);
			const auto whiteSquare = toSquare(0, i);

			state.boards.setPiece(blackSquare, colorPiece(backrank[i], Color::Black));
			state.boards.setPiece(whiteSquare, colorPiece(backrank[i], Color::White));
		}

		m_blackToMove = false;
		m_fullmove = 1;

		regen();

		return true;
	}

	auto Position::resetFromDfrcIndex(u32 n) -> bool
	{
		assert(g_opts.chess960);

		if (n >= 960 * 960)
		{
			std::cerr << "invalid dfrc position index " << n << std::endl;
			return false;
		}

		m_states.resize(1);
		m_keys.clear();

		auto &state = currState();
		state = BoardState{};

		auto &bbs = state.boards.bbs();

		bbs.forPiece(PieceType::Pawn) = U64(0x00FF00000000FF00);

		bbs.forColor(Color::Black) = U64(0x00FF000000000000);
		bbs.forColor(Color::White) = U64(0x000000000000FF00);

		const auto blackBackrank = scharnaglToBackrank(n / 960);
		const auto whiteBackrank = scharnaglToBackrank(n % 960);

		for (i32 i = 0; i < 8; ++i)
		{
			const auto blackSquare = toSquare(7, i);
			const auto whiteSquare = toSquare(0, i);

			state.boards.setPiece(blackSquare, colorPiece(blackBackrank[i], Color::Black));
			state.boards.setPiece(whiteSquare, colorPiece(whiteBackrank[i], Color::White));
		}

		m_blackToMove = false;
		m_fullmove = 1;

		regen();

		return true;
	}

	auto Position::copyStateFrom(const Position &other) -> void
	{
		m_states.clear();
		m_keys.clear();

		m_states.push_back(other.currState());

		m_blackToMove = other.m_blackToMove;
		m_fullmove = other.m_fullmove;
	}

	template <bool UpdateNnue, bool StateHistory>
	auto Position::applyMoveUnchecked(Move move, eval::NnueState *nnueState) -> void
	{
		if constexpr (UpdateNnue)
			assert(nnueState != nullptr);

		auto &prevState = currState();

		if constexpr (StateHistory)
		{
			assert(m_states.size() < m_states.capacity());
			m_states.push_back(prevState);
		}

		m_keys.push_back(prevState.keys.all);

		auto &state = currState();

		m_blackToMove = !m_blackToMove;

		state.keys.flipStm();

		const auto stm = opponent();

		if (stm == Color::Black)
			++m_fullmove;

		if (!move)
		{
			state.pinned = calcPinned();
			state.threats = calcThreats();

			return;
		}

		const auto moveType = move.type();

		const auto moveSrc = move.src();
		const auto moveDst = move.dst();

		const auto moving = state.boards.pieceAt(moveSrc);

		eval::NnueUpdates updates{};
		auto captured = Piece::None;

		switch (moveType)
		{
		case MoveType::Standard:
			captured = movePiece<true, UpdateNnue>(moving, moveSrc, moveDst, updates);
			break;
		case MoveType::Promotion:
			captured = promotePawn<true, UpdateNnue>(moving, moveSrc, moveDst, updates);
			break;
		}

		assert(pieceTypeOrNone(captured) != PieceType::King);

		if constexpr (UpdateNnue)
			nnueState->pushUpdates<!StateHistory>(updates, state.boards.bbs(), state.kings);

		if (captured == Piece::None
			&& pieceType(moving) != PieceType::Pawn)
			++state.halfmove;
		else state.halfmove = 0;

		state.checkers = calcCheckers();
		state.pinned = calcPinned();
		state.threats = calcThreats();
	}

	template <bool UpdateNnue>
	auto Position::popMove(eval::NnueState *nnueState) -> void
	{
		assert(m_states.size() > 1 && "popMove() with no previous move?");

		if constexpr (UpdateNnue)
		{
			assert(nnueState != nullptr);
			nnueState->pop();
		}

		m_states.pop_back();
		m_keys.pop_back();

		m_blackToMove = !m_blackToMove;

		if (toMove() == Color::Black)
			--m_fullmove;
	}

	auto Position::clearStateHistory() -> void
	{
		const auto state = currState();
		m_states.resize(1);
		currState() = state;
	}

	auto Position::isPseudolegal(Move move) const -> bool
	{
		assert(move != NullMove);

		const auto &state = currState();

		const auto us = toMove();

		const auto src = move.src();
		const auto srcPiece = state.boards.pieceAt(src);

		if (srcPiece == Piece::None || pieceColor(srcPiece) != us)
			return false;

		const auto dst = move.dst();
		const auto dstPiece = state.boards.pieceAt(dst);

		// we're capturing something
		if (dstPiece != Piece::None
			// we're capturing our own piece
			&& (pieceColor(dstPiece) == us
				// or trying to capture a king
				|| pieceType(dstPiece) == PieceType::King))
			return false;

		const auto srcPieceType = pieceType(srcPiece);
		const auto them = oppColor(us);
		const auto occ = state.boards.bbs().occupancy();

		if (srcPieceType == PieceType::Pawn)
		{
			const auto srcRank = move.srcRank();
			const auto dstRank = move.dstRank();

			// backwards move
			if ((us == Color::Black && dstRank >= srcRank)
				|| (us == Color::White && dstRank <= srcRank))
				return false;

			const auto promoRank = relativeRank(us, 7);

			// non-promotion move to back rank, or promotion move to any other rank
			if (move.isPromo() != (dstRank == promoRank))
				return false;

			// sideways move
			if (move.srcFile() != move.dstFile())
			{
				// not valid attack
				if (!(attacks::getPawnAttacks(src, us) & state.boards.bbs().forColor(them))[dst])
					return false;
			}
			// forward move onto a piece
			else if (dstPiece != Piece::None)
				return false;

			if (std::abs(dstRank - srcRank) > 1)
				return false;
		}
		else
		{
			if (move.isPromo())
				return false;

			Bitboard attacks{};

			switch (srcPieceType)
			{
			case PieceType:: Alfil: attacks = attacks::getAlfilAttacks(src); break;
			case PieceType::  Ferz: attacks = attacks::getFerzAttacks(src); break;
			case PieceType::Knight: attacks = attacks::getKnightAttacks(src); break;
			case PieceType::  Rook: attacks = attacks::getRookAttacks(src, occ); break;
			case PieceType::  King: attacks = attacks::getKingAttacks(src); break;
			default: __builtin_unreachable();
			}

			if (!attacks[dst])
				return false;
		}

		return true;
	}

	// This does *not* check for pseudolegality, moves are assumed to be pseudolegal
	auto Position::isLegal(Move move) const -> bool
	{
		assert(move != NullMove);

		const auto us = toMove();
		const auto them = oppColor(us);

		const auto &state = currState();
		const auto &bbs = state.boards.bbs();

		const auto src = move.src();
		const auto dst = move.dst();

		const auto king = state.kings.color(us);

		const auto moving = state.boards.pieceAt(src);

		if (pieceType(moving) == PieceType::King)
		{
			const auto kinglessOcc = bbs.occupancy() ^ bbs.kings(us);

			return !state.threats[move.dst()]
				&& (attacks::getRookAttacks(dst, kinglessOcc) & bbs.rooks(them)).empty();
		}

		// multiple checks can only be evaded with a king move
		if (state.checkers.multiple()
			|| state.pinned[src] && !orthoRayIntersecting(src, dst)[king])
			return false;

		if (state.checkers.empty())
			return true;

		const auto checker = state.checkers.lowestSquare();
		return (orthoRayBetween(king, checker) | Bitboard::fromSquare(checker))[dst];
	}

	// see comment in cuckoo.cpp
	auto Position::hasCycle(i32 ply) const -> bool
	{
		const auto &state = currState();

		const auto end = std::min<i32>(state.halfmove, static_cast<i32>(m_keys.size()));

		if (end < 3)
			return false;

		const auto S = [this](i32 d)
		{
			return m_keys[m_keys.size() - d];
		};

		const auto occ = state.boards.bbs().occupancy();
		const auto originalKey = state.keys.all;

		auto other = ~(originalKey ^ S(1));

		for (i32 d = 3; d <= end; d += 2)
		{
			const auto currKey = S(d);

			other ^= ~(currKey ^ S(d - 1));
			if (other != 0)
				continue;

			const auto diff = originalKey ^ currKey;

			u32 slot = cuckoo::h1(diff);

			if (diff != cuckoo::keys[slot])
				slot = cuckoo::h2(diff);

			if (diff != cuckoo::keys[slot])
				continue;

			const auto move = cuckoo::moves[slot];

			if ((occ & orthoRayBetween(move.src(), move.dst())).empty())
			{
				// repetition is after root, done
				if (ply > d)
					return true;

				auto piece = state.boards.pieceAt(move.src());
				if (piece == Piece::None)
					piece = state.boards.pieceAt(move.dst());

				assert(piece != Piece::None);

				return pieceColor(piece) == toMove();
			}
		}

		return false;
	}

	auto Position::isDrawn(bool threefold) const -> bool
	{
		const auto halfmove = currState().halfmove;

		if (halfmove >= 140)
		{
			if (!isCheck())
				return true;

			//TODO there's a speedup possible here, but
			// it requires a lot of movegen refactoring
			ScoredMoveList moves{};
			generateAll(moves, *this);

			return std::ranges::any_of(moves, [this](const auto move)
			{
				return isLegal(move.move);
			});
		}

		const auto currKey = currState().keys.all;
		const auto limit = std::max(0, static_cast<i32>(m_keys.size()) - halfmove - 2);

		i32 repetitionsLeft = threefold ? 2 : 1;

		for (auto i = static_cast<i32>(m_keys.size()) - 4; i >= limit; i -= 2)
		{
			if (m_keys[i] == currKey
				&& --repetitionsLeft == 0)
				return true;
		}

		const auto &bbs = this->bbs();

		// KK
		if (bbs.occupancy() == bbs.kings())
			return true;

		//TODO more?

		return false;
	}

	auto Position::toFen() const -> std::string
	{
		const auto &state = currState();

		std::ostringstream fen{};

		for (i32 rank = 7; rank >= 0; --rank)
		{
			for (i32 file = 0; file < 8; ++file)
			{
				if (state.boards.pieceAt(rank, file) == Piece::None)
				{
					u32 emptySquares = 1;
					for (; file < 7 && state.boards.pieceAt(rank, file + 1) == Piece::None; ++file, ++emptySquares) {}

					fen << static_cast<char>('0' + emptySquares);
				}
				else fen << pieceToChar(state.boards.pieceAt(rank, file));
			}

			if (rank > 0)
				fen << '/';
		}

		fen << (toMove() == Color::White ? " w " : " b ");

		fen << " - -";

		fen << ' ' << state.halfmove;
		fen << ' ' << m_fullmove;

		return fen.str();
	}

	template <bool UpdateKey>
	auto Position::setPiece(Piece piece, Square square) -> void
	{
		assert(piece != Piece::None);
		assert(square != Square::None);

		assert(pieceType(piece) != PieceType::King);

		auto &state = currState();

		state.boards.setPiece(square, piece);

		if constexpr (UpdateKey)
			state.keys.flipPiece(piece, square);
	}

	template <bool UpdateKey>
	auto Position::removePiece(Piece piece, Square square) -> void
	{
		assert(piece != Piece::None);
		assert(square != Square::None);

		assert(pieceType(piece) != PieceType::King);

		auto &state = currState();

		state.boards.removePiece(square, piece);

		if constexpr (UpdateKey)
			state.keys.flipPiece(piece, square);
	}

	template <bool UpdateKey>
	auto Position::movePieceNoCap(Piece piece, Square src, Square dst) -> void
	{
		assert(piece != Piece::None);

		assert(src != Square::None);
		assert(dst != Square::None);

		if (src == dst)
			return;

		auto &state = currState();

		state.boards.movePiece(src, dst, piece);

		if (pieceType(piece) == PieceType::King)
		{
			const auto color = pieceColor(piece);
			state.kings.color(color) = dst;
		}

		if constexpr (UpdateKey)
			state.keys.movePiece(piece, src, dst);
	}

	template <bool UpdateKey, bool UpdateNnue>
	auto Position::movePiece(Piece piece, Square src, Square dst, eval::NnueUpdates &nnueUpdates) -> Piece
	{
		assert(piece != Piece::None);

		assert(src != Square::None);
		assert(dst != Square::None);
		assert(src != dst);

		auto &state = currState();

		const auto captured = state.boards.pieceAt(dst);

		if (captured != Piece::None)
		{
			assert(pieceType(captured) != PieceType::King);

			state.boards.removePiece(dst, captured);

			// NNUE update done below

			if constexpr (UpdateKey)
				state.keys.flipPiece(captured, dst);
		}

		state.boards.movePiece(src, dst, piece);

		if (pieceType(piece) == PieceType::King)
		{
			const auto color = pieceColor(piece);

			if constexpr (UpdateNnue)
			{
				if (eval::InputFeatureSet::refreshRequired(color, state.kings.color(color), dst))
					nnueUpdates.setRefresh(color);
			}

			state.kings.color(color) = dst;
		}

		if constexpr (UpdateNnue)
		{
			nnueUpdates.pushSubAdd(piece, src, dst);

			if (captured != Piece::None)
				nnueUpdates.pushSub(captured, dst);
		}

		if constexpr (UpdateKey)
			state.keys.movePiece(piece, src, dst);

		return captured;
	}

	template <bool UpdateKey, bool UpdateNnue>
	auto Position::promotePawn(Piece pawn, Square src, Square dst, eval::NnueUpdates &nnueUpdates) -> Piece
	{
		assert(pawn != Piece::None);
		assert(pieceType(pawn) == PieceType::Pawn);

		assert(src != Square::None);
		assert(dst != Square::None);
		assert(src != dst);

		assert(squareRank(dst) == relativeRank(pieceColor(pawn), 7));
		assert(squareRank(src) == relativeRank(pieceColor(pawn), 6));

		auto &state = currState();

		const auto captured = state.boards.pieceAt(dst);

		if (captured != Piece::None)
		{
			assert(pieceType(captured) != PieceType::King);

			state.boards.removePiece(dst, captured);

			if constexpr (UpdateNnue)
				nnueUpdates.pushSub(captured, dst);

			if constexpr (UpdateKey)
				state.keys.flipPiece(captured, dst);
		}

		state.boards.moveAndChangePiece(src, dst, pawn, PieceType::Ferz);

		if constexpr(UpdateNnue || UpdateKey)
		{
			const auto coloredFerz = copyPieceColor(pawn, PieceType::Ferz);

			if constexpr (UpdateNnue)
			{
				nnueUpdates.pushSub(pawn, src);
				nnueUpdates.pushAdd(coloredFerz, dst);
			}

			if constexpr (UpdateKey)
			{
				state.keys.flipPiece(pawn, src);
				state.keys.flipPiece(coloredFerz, dst);
			}
		}

		return captured;
	}

	auto Position::regen() -> void
	{
		auto &state = currState();

		state.boards.regenFromBbs();

		state.keys.clear();

		for (u32 rank = 0; rank < 8; ++rank)
		{
			for (u32 file = 0; file < 8; ++file)
			{
				const auto square = toSquare(rank, file);
				if (const auto piece = state.boards.pieceAt(square); piece != Piece::None)
				{
					if (pieceType(piece) == PieceType::King)
						state.kings.color(pieceColor(piece)) = square;

					state.keys.flipPiece(piece, toSquare(rank, file));
				}
			}
		}

		if (toMove() == Color::Black)
			state.keys.flipStm();

		state.checkers = calcCheckers();
		state.pinned = calcPinned();
		state.threats = calcThreats();
	}

	auto Position::moveFromUci(const std::string &move) const -> Move
	{
		if (move.length() < 4 || move.length() > 5)
			return NullMove;

		if (move.length() == 5 && move[4] != 'q')
			return NullMove;

		const auto src = squareFromString(move.substr(0, 2));
		const auto dst = squareFromString(move.substr(2, 2));

		const auto &state = currState();

		const auto srcPiece = pieceType(state.boards.pieceAt(src));
		const auto promoRank = relativeRank(toMove(), 7);

		return (srcPiece == PieceType::Pawn && squareRank(dst) == promoRank)
			? Move::promotion(src, dst)
			: Move:: standard(src, dst);
	}

	auto Position::starting() -> Position
	{
		Position position{};
		position.resetToStarting();
		return position;
	}

	auto Position::fromFen(const std::string &fen) -> std::optional<Position>
	{
		Position position{};

		if (position.resetFromFen(fen))
			return position;

		return {};
	}

	auto Position::fromFrcIndex(u32 n) -> std::optional<Position>
	{
		assert(g_opts.chess960);

		if (n >= 960)
		{
			std::cerr << "invalid frc position index " << n << std::endl;
			return {};
		}

		Position position{};
		position.resetFromFrcIndex(n);

		return position;
	}

	auto Position::fromDfrcIndex(u32 n) -> std::optional<Position>
	{
		assert(g_opts.chess960);

		if (n >= 960 * 960)
		{
			std::cerr << "invalid dfrc position index " << n << std::endl;
			return {};
		}

		Position position{};
		position.resetFromDfrcIndex(n);

		return position;
	}

	auto squareFromString(const std::string &str) -> Square
	{
		if (str.length() != 2)
			return Square::None;

		const auto file = str[0];
		const auto rank = str[1];

		if (file < 'a' || file > 'h'
			|| rank < '1' || rank > '8')
			return Square::None;

		return toSquare(static_cast<u32>(rank - '1'), static_cast<u32>(file - 'a'));
	}
}
