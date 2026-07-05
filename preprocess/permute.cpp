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

#include "../src/types.h"

#include <fstream>
#include <iterator>
#include <memory>
#include <type_traits>

#include "../src/eval/arch.h"
#include "../src/eval/header.h"
#include "../src/eval/nnue.h"
#include "../src/util/multi_array.h"

using namespace oranj;
using namespace oranj::eval;

using PsqWeightType = Network::FeatureTransformer::PsqWeightType;
using ThreatWeightType = Network::FeatureTransformer::ThreatWeightType;
using FtBiasType = Network::FeatureTransformer::OutputType;

using L1WeightType = LayeredArch::L1WeightType;
using L1BiasType = LayeredArch::L1BiasType;
using L2WeightType = LayeredArch::L2WeightType;
using L2BiasType = LayeredArch::L2BiasType;
using L3WeightType = LayeredArch::L3WeightType;
using L3BiasType = LayeredArch::L3BiasType;

struct ThreatInputFtWeights {
    util::MultiArray<PsqWeightType, InputFeatureSet::kBucketCount * InputFeatureSet::kInputSize * kL1Size> psq;
    util::MultiArray<ThreatWeightType, InputFeatureSet::kThreatFeatures * kL1Size> threat;
};

struct NoThreatInputFtWeights {
    util::MultiArray<PsqWeightType, InputFeatureSet::kBucketCount * InputFeatureSet::kInputSize * kL1Size> psq;
    static std::array<ThreatWeightType, 0> threat;
};

using FtWeights = std::conditional_t<InputFeatureSet::kThreatInputs, ThreatInputFtWeights, NoThreatInputFtWeights>;

constexpr auto kL1Weights = kL1Size * (1 + !LayeredArch::kPairwise);
constexpr auto kL2Weights = kL2Size * (1 + kDualActivation);

struct LoadedNetwork {
    FtWeights ftWeights;
    std::array<FtBiasType, kL1Size> ftBiases;
    std::array<L1WeightType, OutputBucketing::kBucketCount * kL1Weights * kL2Size> l1Weights;
    std::array<L1BiasType, OutputBucketing::kBucketCount * kL2Size> l1Biases;
    std::array<L2WeightType, OutputBucketing::kBucketCount * kL2Weights * kL3Size> l2Weights;
    std::array<L2BiasType, OutputBucketing::kBucketCount * kL3Size> l2Biases;
    std::array<L3WeightType, OutputBucketing::kBucketCount * kL3Size> l3Weights;
    std::array<L3BiasType, OutputBucketing::kBucketCount> l3Biases;
};

i32 main(i32 argc, char* argv[]) {
    if (argc < 3) {
        eprintln("usage: {} <input net> <output net>", argv[0]);
        return 1;
    }

    std::ifstream in{argv[1], std::ios::binary};
    if (!in) {
        eprintln("Failed to open input file \"{}\"", argv[1]);
        return 1;
    }

    NetworkHeader header;
    if (!in.read(reinterpret_cast<char*>(&header), sizeof(header))) {
        eprintln("Failed to read network header");
        return 1;
    }

    if (header.magic != std::array{'C', 'B', 'N', 'F'}) {
        eprintln("Invalid header magic");
        return 1;
    }

    std::ofstream out{argv[2], std::ios::binary};
    if (!out) {
        eprintln("Failed to open output file \"{}\"", argv[2]);
        return 1;
    }

    if (!out.write(reinterpret_cast<const char*>(&header), sizeof(header))) {
        eprintln("Failed to write header");
        return 1;
    }

    if (testFlags(header.flags, NetworkFlags::kZstdCompressed)) {
        println("Compressed network, skipping permutation");
        std::copy(std::istreambuf_iterator{in}, std::istreambuf_iterator<char>{}, std::ostreambuf_iterator{out});
        if (!out) {
            eprintln("Failed to write network");
            return 1;
        }
        return 0;
    }

    if constexpr (!LayeredArch::kRequiresFtPermute) {
        println("No permutation required for current network arch");
        std::copy(std::istreambuf_iterator{in}, std::istreambuf_iterator<char>{}, std::ostreambuf_iterator{out});
        if (!out) {
            eprintln("Failed to write network");
            return 1;
        }
        return 0;
    }

    auto network = std::make_unique<LoadedNetwork>();

    if (!in.read(reinterpret_cast<char*>(network.get()), sizeof(LoadedNetwork))) {
        eprintln("Failed to read network");
        return 1;
    }

    println("Permuting network");

    LayeredArch::permuteParams<i16>(network->ftWeights.psq);
    LayeredArch::permuteParams<i16>(network->ftBiases);

    if constexpr (InputFeatureSet::kThreatInputs) {
        LayeredArch::permuteParams<i8>(network->ftWeights.threat);
    }

    if (!out.write(reinterpret_cast<const char*>(network.get()), sizeof(LoadedNetwork))) {
        eprintln("Failed to write network");
        return 1;
    }

    return 0;
}
