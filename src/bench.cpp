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

#include "bench.h"

#include <array>

#include "position/position.h"

namespace oranj::bench
{
	auto run(search::Searcher &searcher, i32 depth) -> void
	{
		const std::array Fens { // fens from alexandria, ultimately from bitgenie
			"r5r1/1k6/1pqb4/1Bppn1p1/P1n1p2p/P1N1P2P/2KQ1p2/1RBR2N1 w - - 0 45",
			"8/1R6/4q3/3Nk1p1/2P3p1/3PK3/8/8 w - - 2 83",
			"8/8/8/1KQQQ3/2P3qP/5k2/7b/8 b - - 20 76",
			"r2k1r2/4n1p1/p1n1q2p/1pP1p3/1b1p4/B2B2PP/P2PNN1R/K1R1Q3 w - - 1 25",
			"2r1r3/p1pk1ppp/bpnpp2b/8/3P4/BPQ1PN1P/P1P1KPP1/R6R b - - 1 14",
			"3kq3/3p4/3p1p2/6pK/1R1Q4/1P1B1r2/8/8 w - - 2 44",
			"1nbkq3/1rpppr1p/3b1p2/p1PP1Pp1/1p6/PP1NP1PB/3Q3n/RNBKR3 w - - 0 20",
			"r4br1/8/p2k2qp/7n/1R1N4/3BB1P1/P2PPQ1P/3K4 w - - 3 32",
			"8/8/8/8/3Qk1n1/2K1P3/8/8 b - - 46 162",
			"r1bk3r/2p3pp/ppn1qp1b/3pp3/8/P1NBBPP1/1PPPP2P/2RKQR2 w - - 0 14",
			"6r1/r5p1/pk6/1p2pNN1/n2p4/B6P/P1R5/KB2Q3 b - - 0 42",
			"rnbkqbnr/ppppp1p1/5p1p/8/8/3P2P1/PPP1PP1P/RNBKQBNR w - - 0 1",
			"5b1r/8/1p1pq1p1/p1k3P1/5RP1/P1PB4/4KQ2/8 w - - 1 44",
			"2r1qr2/8/1pkp2pb/p2pn1N1/3R2PP/3BP1Q1/P1P1R3/2K5 b - - 6 30",
			"1r6/1r6/k1p3pn/1p1n1p1p/pPbPpP1P/Pq2Q1PN/1K1N2R1/2B1RB2 b - - 2 104",
			"8/7p/1k1K2p1/8/2P3P1/p3B2P/1r6/8 b - - 1 49",
			"1r2q3/R4pn1/1p1pkn2/3p1p2/1PpP2p1/N1P1K1P1/3Q3P/2B1R3 b - - 5 31",
			"8/1Q6/3Q4/3p1p2/2pkq2R/5q2/5K2/8 w - - 2 116",
			"8/2p4p/b7/4Qp2/4kP2/P1K5/8/8 b - - 15 55",
			"8/4k3/4R3/2PK4/1P3Nn1/P2PPn2/5r2/8 b - - 2 58",
		};

		searcher.newGame();

		usize nodes{};
		f64 time{};

		Position pos{};

		for (const auto &fen : Fens)
		{
			if (!pos.resetFromFen(fen))
				return;

			search::BenchData data{};
			searcher.runBench(data, pos, depth);

			nodes += data.search.nodes;
			time += data.time;
		}

		std::cout << "info string " << time << " seconds" << std::endl;
		std::cout << nodes << " nodes " << static_cast<usize>(static_cast<f64>(nodes) / time) << " nps" << std::endl;
	}
}
