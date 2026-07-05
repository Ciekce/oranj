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

#include "../types.h"

#include <array>

#include "nnue/activation.h"
#include "nnue/arch/multilayer.h"
#include "nnue/arch/singlelayer.h"
#include "nnue/features/psq.h"
#include "nnue/features/threats.h"
#include "nnue/output.h"

namespace oranj::eval {
    // current arch: ((704x16+59808+pp)hm->640)x2->(32x2->64->1)x8
    // pairwise clipped ReLU -> dual clipped + clipped squared ReLU -> clipped ReLU,
    // skip connection over L2

    constexpr u32 kFtQBits = 8;
    constexpr u32 kL1QBits = 6;

    constexpr u32 kFtScaleBits = 7;

    constexpr u32 kL1Size = 128;
    constexpr u32 kL2Size = 1;
    constexpr u32 kL3Size = 0;

    using L1Activation = nnue::activation::SquaredClippedReLU;

    constexpr bool kDualActivation = false;
    constexpr bool kSkipL2 = false;

    constexpr i32 kScale = 400;

    using PsqFeatureSet = nnue::features::psq::SingleBucket;

    using InputFeatureSet = PsqFeatureSet;

    using OutputBucketing = nnue::output::Single;

    using LayeredArch = nnue::arch::SingleLayer<
        InputFeatureSet,
        kL1Size,
        (1 << kFtQBits) - 1,
        1 << kL1QBits,
        L1Activation,
        OutputBucketing,
        kScale>;
} // namespace oranj::eval
