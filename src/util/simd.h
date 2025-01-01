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

#include <cassert>

#include "../arch.h"

#if OJ_HAS_AVX512
#include "simd/avx512.h"
#elif OJ_HAS_AVX2
#include "simd/avx2.h"
#elif OJ_HAS_SSE41
#include "simd/sse41.h"
#elif OJ_HAS_NEON
#include "simd/neon.h"
#else
#include "simd/none.h"
#endif

#define OJ_SIMD_ALIGNAS alignas(oranj::util::simd::Alignment)

namespace oranj::util::simd
{
	constexpr usize ChunkSize = sizeof(VectorI16) / sizeof(i16);

	template <typename T = void>
	auto isAligned(const T *ptr)
	{
		return util::isAligned<Alignment>(ptr);
	}

	template <typename T>
	struct VectorImpl {};

	template <>
	struct VectorImpl<i16>
	{
		using Type = VectorI16;
	};

	template <>
	struct VectorImpl<i32>
	{
		using Type = VectorI32;
	};

	template <typename T>
	using Vector = typename VectorImpl<T>::Type;
	template <typename T>
	struct PromotedVectorImpl {};

	template <>
	struct PromotedVectorImpl<i16>
	{
		using Type = VectorI32;
	};

	template <typename T>
	using PromotedVector = typename PromotedVectorImpl<T>::Type;

#define OJ_SIMD_OP_0(Name) \
	template <typename T> \
	OJ_ALWAYS_INLINE_NDEBUG inline auto Name() = delete; \
	template <> \
	OJ_ALWAYS_INLINE_NDEBUG inline auto Name<i16>() \
	{ \
		return impl::Name##I16(); \
	} \
	template <> \
	OJ_ALWAYS_INLINE_NDEBUG inline auto Name<i32>() \
	{ \
		return impl::Name##I32(); \
	}

#define OJ_SIMD_OP_1_VALUE(Name, Arg0) \
	template <typename T> \
	OJ_ALWAYS_INLINE_NDEBUG inline auto Name(T Arg0) = delete; \
	template <> \
	OJ_ALWAYS_INLINE_NDEBUG inline auto Name<i16>(i16 Arg0) \
	{ \
		return impl::Name##I16(Arg0); \
	} \
	template <> \
	OJ_ALWAYS_INLINE_NDEBUG inline auto Name<i32>(i32 Arg0) \
	{ \
		return impl::Name##I32(Arg0); \
	}

#define OJ_SIMD_OP_2_VECTORS(Name, Arg0, Arg1) \
	template <typename T> \
	OJ_ALWAYS_INLINE_NDEBUG inline auto Name(Vector<T> Arg0, Vector<T> Arg1) = delete; \
	template <> \
	OJ_ALWAYS_INLINE_NDEBUG inline auto Name<i16>(Vector<i16> Arg0, Vector<i16> Arg1) \
	{ \
		return impl::Name##I16(Arg0, Arg1); \
	} \
	template <> \
	OJ_ALWAYS_INLINE_NDEBUG inline auto Name<i32>(Vector<i32> Arg0, Vector<i32> Arg1) \
	{ \
		return impl::Name##I32(Arg0, Arg1); \
	}

#define OJ_SIMD_OP_3_VECTORS(Name, Arg0, Arg1, Arg2) \
	template <typename T> \
	OJ_ALWAYS_INLINE_NDEBUG inline auto Name(Vector<T> Arg0, Vector<T> Arg1, Vector<T> Arg2) = delete; \
	template <> \
	OJ_ALWAYS_INLINE_NDEBUG inline auto Name<i16>(Vector<i16> Arg0, Vector<i16> Arg1, Vector<i16> Arg2) \
	{ \
		return impl::Name##I16(Arg0, Arg1, Arg2); \
	} \
	template <> \
	OJ_ALWAYS_INLINE_NDEBUG inline auto Name<i32>(Vector<i32> Arg0, Vector<i32> Arg1, Vector<i32> Arg2) \
	{ \
		return impl::Name##I32(Arg0, Arg1, Arg2); \
	}

OJ_SIMD_OP_0(zero)
OJ_SIMD_OP_1_VALUE(set1, v)
OJ_SIMD_OP_2_VECTORS(add, a, b)
OJ_SIMD_OP_2_VECTORS(sub, a, b)
OJ_SIMD_OP_2_VECTORS(mul, a, b)
OJ_SIMD_OP_2_VECTORS(min, a, b)
OJ_SIMD_OP_2_VECTORS(max, a, b)
OJ_SIMD_OP_3_VECTORS(clamp, v, min, max)

	template <typename T>
	OJ_ALWAYS_INLINE_NDEBUG inline auto load(const void *ptr) = delete;
	template <>
	OJ_ALWAYS_INLINE_NDEBUG inline auto load<i16>(const void *ptr)
	{
		return impl::loadI16(ptr);
	}
	template <>
	OJ_ALWAYS_INLINE_NDEBUG inline auto load<i32>(const void *ptr)
	{
		return impl::loadI32(ptr);
	}

	template <typename T>
	OJ_ALWAYS_INLINE_NDEBUG inline auto store(void *ptr, Vector<T> v) = delete;
	template <>
	OJ_ALWAYS_INLINE_NDEBUG inline auto store<i16>(void *ptr, Vector<i16> v)
	{
		impl::storeI16(ptr, v);
	}
	template <>
	OJ_ALWAYS_INLINE_NDEBUG inline auto store<i32>(void *ptr, Vector<i32> v)
	{
		impl::storeI32(ptr, v);
	}

	template <typename T>
	OJ_ALWAYS_INLINE_NDEBUG inline auto mulAddAdj(Vector<T> a, Vector<T> b) = delete;
	template <>
	OJ_ALWAYS_INLINE_NDEBUG inline auto mulAddAdj<i16>(Vector<i16> a, Vector<i16> b)
	{
		return impl::mulAddAdjI16(a, b);
	}

	template <typename T>
	OJ_ALWAYS_INLINE_NDEBUG inline auto mulAddAdjAcc(PromotedVector<T> sum, Vector<T> a, Vector<T> b) = delete;
	template <>
	OJ_ALWAYS_INLINE_NDEBUG inline auto mulAddAdjAcc<i16>(PromotedVector<i16> sum, Vector<i16> a, Vector<i16> b)
	{
		return impl::mulAddAdjAccI16(sum, a, b);
	}

	template <typename T>
	OJ_ALWAYS_INLINE_NDEBUG inline auto hsum(Vector<T> v) = delete;
	template <>
	OJ_ALWAYS_INLINE_NDEBUG inline auto hsum<i32>(Vector<i32> v)
	{
		return impl::hsumI32(v);
	}

#undef OJ_SIMD_OP_0
#undef OJ_SIMD_OP_1_VALUE
#undef OJ_SIMD_OP_2_VECTORS
#undef OJ_SIMD_OP_3_VECTORS
}
