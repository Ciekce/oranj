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

#include <cstdlib>

namespace oranj::util
{
	template <std::uintptr_t Alignment, typename T = void>
	constexpr auto isAligned(const T *ptr)
	{
		return (reinterpret_cast<std::uintptr_t>(ptr) % Alignment) == 0;
	}

	template <typename T>
	inline auto alignedAlloc(usize alignment, usize count)
	{
		const auto size = count * sizeof(T);

#ifdef _MSC_VER
		return static_cast<T *>(_aligned_malloc(size, alignment));
#else
		return static_cast<T *>(std::aligned_alloc(alignment, size));
#endif
	}

	inline auto alignedFree(void *ptr)
	{
		if (!ptr)
			return;

#ifdef _MSC_VER
		_aligned_free(ptr);
#else
		std::free(ptr);
#endif
	}
}
