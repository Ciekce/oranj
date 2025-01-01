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

#define OJ_ENUM_FLAG_OPERATOR(T,UT,O) \
[[maybe_unused]] inline constexpr auto operator O(T lhs, T rhs) \
{ \
	return static_cast<T>(static_cast<UT>(lhs) O static_cast<UT>(rhs)); \
} \
[[maybe_unused]] inline constexpr auto operator O##=(T &lhs, T rhs) \
{ \
	return lhs = static_cast<T>(static_cast<UT>(lhs) O static_cast<UT>(rhs)); \
}

#define OJ_ENUM_FLAGS(UT,T) \
enum class T : UT; \
[[maybe_unused]] inline constexpr auto operator ~(T t) { return static_cast<T>(~static_cast<UT>(t)); } \
OJ_ENUM_FLAG_OPERATOR(T, UT, |) \
OJ_ENUM_FLAG_OPERATOR(T, UT, ^) \
OJ_ENUM_FLAG_OPERATOR(T, UT, &) \
enum class T : UT

namespace oranj
{
	template <typename T>
	constexpr auto testFlags(T field, T flags)
	{
		return (field & flags) != T::None;
	}

	template <typename T>
	constexpr auto setFlags(T field, T flags, bool v)
	{
		if (v)
			field |= flags;
		else field &= ~flags;

		return field;
	}

	template <typename T>
	constexpr auto flipFlags(T field, T flags)
	{
		return field ^ flags;
	}
}
