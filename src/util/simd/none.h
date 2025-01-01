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

#include "../../types.h"

#include "../../arch.h"

#if !OJ_HAS_SIMD

#warning Falling back to autovectorization - expect an extremely slow binary

#include <cmath>
#include <algorithm>

namespace oranj::util::simd
{
	using VectorI16 = i16;
	using VectorI32 = i32;

	constexpr std::uintptr_t Alignment = 8;

	namespace impl
	{
		OJ_ALWAYS_INLINE_NDEBUG inline auto zeroI16() -> VectorI16
		{
			return 0;
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto set1I16(i16 v) -> VectorI16
		{
			return v;
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto loadI16(const void *ptr) -> VectorI16
		{
			return *static_cast<const VectorI16 *>(ptr);
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto storeI16(void *ptr, VectorI16 v)
		{
			*static_cast<VectorI16 *>(ptr) = v;
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto minI16(VectorI16 a, VectorI16 b) -> VectorI16
		{
			return std::min(a, b);
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto maxI16(VectorI16 a, VectorI16 b) -> VectorI16
		{
			return std::max(a, b);
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto clampI16(
			VectorI16 v, VectorI16 min, VectorI16 max) -> VectorI16
		{
			return std::clamp(v, min, max);
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto addI16(VectorI16 a, VectorI16 b) -> VectorI16
		{
			return static_cast<VectorI16>(a + b);
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto subI16(VectorI16 a, VectorI16 b) -> VectorI16
		{
			return static_cast<VectorI16>(a - b);
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto mulI16(VectorI16 a, VectorI16 b) -> VectorI16
		{
			//TODO is this correct for overflow?
			return static_cast<VectorI16>(a * b);
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto mulAddAdjI16(VectorI16 a, VectorI16 b) -> VectorI32
		{
			return static_cast<VectorI32>(a) * static_cast<VectorI32>(b);
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto zeroI32() -> VectorI32
		{
			return 0;
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto set1I32(i32 v) -> VectorI32
		{
			return v;
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto loadI32(const void *ptr) -> VectorI32
		{
			return *static_cast<const VectorI32 *>(ptr);
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto storeI32(void *ptr, VectorI32 v)
		{
			*static_cast<VectorI32 *>(ptr) = v;
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto minI32(VectorI32 a, VectorI32 b) -> VectorI32
		{
			return std::min(a, b);
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto maxI32(VectorI32 a, VectorI32 b) -> VectorI32
		{
			return std::max(a, b);
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto clampI32(
			VectorI32 v, VectorI32 min, VectorI32 max) -> VectorI32
		{
			return std::clamp(v, min, max);
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto addI32(VectorI32 a, VectorI32 b) -> VectorI32
		{
			return a + b;
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto subI32(VectorI32 a, VectorI32 b) -> VectorI32
		{
			return a - b;
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto mulI32(VectorI32 a, VectorI32 b) -> VectorI32
		{
			return a * b;
		}

		OJ_ALWAYS_INLINE_NDEBUG inline auto hsumI32(VectorI32 v) -> i32
		{
			return v;
		}

		// Depends on addI32
		OJ_ALWAYS_INLINE_NDEBUG inline auto mulAddAdjAccI16(VectorI32 sum, VectorI16 a, VectorI16 b) -> VectorI32
		{
			const auto products = mulAddAdjI16(a, b);
			return addI32(sum, products);
		}
	}
}

#endif
