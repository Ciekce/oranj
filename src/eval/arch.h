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

#include "../types.h"

#include <array>

#include "nnue/activation.h"
#include "nnue/arch/multilayer.h"
#include "nnue/arch/singlelayer.h"
#include "nnue/features.h"
#include "nnue/output.h"

namespace oranj::eval {
    // current arch: (768->128)x2->1
    // squared clipped ReLU

    constexpr u32 kFtQBits = 8;
    constexpr u32 kL1QBits = 6;

    constexpr u32 kL1Size = 128;

    using L1Activation = nnue::activation::SquaredClippedReLU;

    constexpr i32 kScale = 400;

    using InputFeatureSet = nnue::features::SingleBucket;

    using OutputBucketing = nnue::output::Single;

    using LayeredArch =
        nnue::arch::SingleLayer<kL1Size, (1 << kFtQBits) - 1, 1 << kL1QBits, L1Activation, OutputBucketing, kScale>;
} // namespace oranj::eval
