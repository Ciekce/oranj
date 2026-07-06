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

#include "bench.h"

#include <array>
#include <string_view>

#if OJ_SPARSE_BENCH_L1_SIZE > 0
    #include <fmt/ostream.h>
    #include <fstream>
#endif

#include "opts.h"
#include "position.h"
#include "util/numa/numa.h"

namespace oranj::bench {
    using namespace std::string_view_literals;

    // from prolix
    constexpr std::array kFens = {
        "r5r1/1k6/1pqb4/1Bppn1p1/P1n1p2p/P1N1P2P/2KQ1p2/1RBR2N1 w - - 0 45"sv,
        "8/1R6/4q3/3Nk1p1/2P3p1/3PK3/8/8 w - - 2 83"sv,
        "8/8/8/1KQQQ3/2P3qP/5k2/7b/8 b - - 20 76"sv,
        "2r1r3/p1pk1ppp/bpnpp2b/8/3P4/BPQ1PN1P/P1P1KPP1/R6R b - - 1 14"sv,
        "3kq3/3p4/3p1p2/6pK/1R1Q4/1P1B1r2/8/8 w - - 2 44"sv,
        "1nbkq3/1rpppr1p/3b1p2/p1PP1Pp1/1p6/PP1NP1PB/3Q3n/RNBKR3 w - - 0 20"sv,
        "r4br1/8/p2k2qp/7n/1R1N4/3BB1P1/P2PPQ1P/3K4 w - - 3 32"sv,
        "8/8/8/8/3Qk1n1/2K1P3/8/8 b - - 46 162"sv,
        "rnbkqbnr/ppppp1p1/5p1p/8/8/3P2P1/PPP1PP1P/RNBKQBNR w - - 0 1"sv,
        "5b1r/8/1p1pq1p1/p1k3P1/5RP1/P1PB4/4KQ2/8 w - - 1 44"sv,
        "2r1qr2/8/1pkp2pb/p2pn1N1/3R2PP/3BP1Q1/P1P1R3/2K5 b - - 6 30"sv,
        "1r2q3/R4pn1/1p1pkn2/3p1p2/1PpP2p1/N1P1K1P1/3Q3P/2B1R3 b - - 5 31"sv,
        "8/1Q6/3Q4/3p1p2/2pkq2R/5q2/5K2/8 w - - 2 116"sv,
        "8/4k3/4R3/2PK4/1P3Nn1/P2PPn2/5r2/8 b - - 2 58"sv,
    };

    void run(i32 depth, usize ttSize) {
        if (!eval::isNetworkLoaded()) {
            eprintln("No network loaded");
            return;
        }

        const auto prevMinimal = g_opts.minimal;

        numa::bindThread(0);

        search::Searcher searcher{ttSize};

        searcher.setLimiter(limit::SearchLimiter{util::Instant::now()});
        searcher.setMaxDepth(depth);

        auto& thread = searcher.take();

        opts::mutableOpts().minimal = true;

        searcher.newGame();

        f64 time{};
        usize nodes{};

        const auto benchPosition = [&](std::string_view fen) {
            println("fen: {}", fen);

            thread.rootPos = *Position::fromFen(fen);

            search::BenchData data{};
            searcher.runBenchSearch(data);

            time += data.time;
            nodes += data.nodes;

            println();
        };

        for (const auto fen : kFens) {
            benchPosition(fen);
        }

        opts::mutableOpts().minimal = prevMinimal;

        println("{:.3f} seconds", time);
        println("{} nodes {} nps", nodes, static_cast<usize>(static_cast<f64>(nodes) / time));

        stats::print();

#if OJ_SPARSE_BENCH_L1_SIZE > 0
        std::ofstream stream{"activations.txt", std::ios::binary};

        bool first = true;
        for (const auto count : eval::nnue::arch::sparse::g_activationCounts) {
            if (!first) {
                fmt::print(stream, ", ");
            }
            fmt::print(stream, "{}", count);
            first = false;
        }

        fmt::println(stream, "");

        println("Wrote FT activation counts to activations.txt");
#endif
    }
} // namespace oranj::bench
