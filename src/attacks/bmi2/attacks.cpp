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

#include "../attacks.h"

#if OJ_HAS_BMI2
namespace oranj::attacks
{
	using namespace bmi2;

	namespace
	{
		auto generateRookAttacks()
		{
			std::array<u16, RookData.tableSize> dst{};

			for (u32 square = 0; square < 64; ++square)
			{
				const auto &data = RookData.data[square];
				const auto entries = 1 << data.srcMask.popcount();

				for (u32 i = 0; i < entries; ++i)
				{
					const auto occupancy = util::pdep(i, data.srcMask);

					Bitboard attacks{};

					for (const auto dir : {offsets::Up, offsets::Down, offsets::Left, offsets::Right})
					{
						attacks |= internal::generateSlidingAttacks(static_cast<Square>(square), dir, occupancy);
					}

					dst[data.offset + i] = static_cast<u16>(util::pext(attacks, data.dstMask));
				}
			}

			return dst;
		}
	}

	const std::array<u16, RookData.tableSize> RookAttacks = generateRookAttacks();
}
#endif // OJ_HAS_BMI2
