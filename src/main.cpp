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
#include "cuckoo.h"
#include "datagen/datagen.h"
#include "eval/nnue.h"
#include "tunable.h"
#include "uci.h"
#include "util/ctrlc.h"
#include "util/parse.h"

#if OJ_EXTERNAL_TUNE
    #include "util/split.h"
#endif

using namespace oranj;

i32 main(i32 argc, const char* argv[]) {
    util::signal::init();

    tunable::init();
    cuckoo::init();

    eval::loadDefaultNetwork();

    if (argc > 1) {
        const std::string_view mode{argv[1]};

        if (mode == "bench") {
            search::Searcher searcher{bench::kDefaultBenchTtSize};
            bench::run(searcher);

            return 0;
        } else if (mode == "datagen") {
            const auto printUsage = [&]() {
                eprintln("usage: {} datagen <marlinformat/viriformat/fen> <path> [threads]", argv[0]);
            };

            if (argc < 4) {
                printUsage();
                return 1;
            }

            u32 threads = 1;
            if (argc > 4 && !util::tryParse<u32>(threads, argv[4])) {
                eprintln("invalid number of threads {}", argv[4]);
                printUsage();
                return 1;
            }

            return datagen::run(printUsage, argv[2], argv[3], static_cast<i32>(threads));
        }
#if OJ_EXTERNAL_TUNE
        else if (mode == "printwf" || mode == "printctt" || mode == "printob")
        {
            if (argc == 2) {
                return 0;
            }

            std::vector<std::string_view> params{};
            split::split(params, argv[2], ',');

            if (mode == "printwf") {
                uci::printWfTuningParams(params);
            } else if (mode == "printctt") {
                uci::printCttTuningParams(params);
            } else if (mode == "printob") {
                uci::printObTuningParams(params);
            }

            return 0;
        }
#endif
    }

    return uci::run();
}
