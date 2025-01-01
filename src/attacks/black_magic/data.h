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

#include <array>

#include "../../core.h"
#include "../../bitboard.h"
#include "../util.h"

namespace oranj::attacks::black_magic
{
	//TODO better magics exist
	constexpr auto Shifts = std::array {
		52, 53, 53, 53, 53, 53, 53, 52,
		53, 54, 54, 54, 54, 54, 54, 53,
		53, 54, 54, 54, 54, 54, 54, 53,
		53, 54, 54, 54, 54, 54, 54, 53,
		53, 54, 54, 54, 54, 54, 54, 53,
		53, 54, 54, 54, 54, 54, 54, 53,
		53, 54, 54, 54, 54, 54, 54, 53,
		52, 53, 53, 53, 53, 53, 53, 52
	};

	constexpr auto Magics = std::array {
		U64(0x2080002040068490), U64(0x06C0021001200C40), U64(0x288009300280A000), U64(0x0100089521003000),
		U64(0x6100040801003082), U64(0x65FFEBC5FFEEE7F0), U64(0x0400080C10219112), U64(0x0200014434060003),
		U64(0x96CD8008C00379D9), U64(0x2A06002101FF81CF), U64(0x7BCA0020802E0641), U64(0xDAE2FFEFFD0020BA),
		U64(0x62E20005E0D200AA), U64(0x2302000830DA0044), U64(0xE81C002CE40A3028), U64(0xC829FFFAFD8BBC06),
		U64(0x12C57E800740089D), U64(0xA574FDFFE13A81FD), U64(0xF331B1FFE0BF79FE), U64(0x0000A1003001010A),
		U64(0x7CD4E2000600264F), U64(0x0299010004000228), U64(0xA36CEBFFAE0FA825), U64(0x9A87E9FFF4408405),
		U64(0x0BAEC0007FF8EB82), U64(0xF81909BDFFE18205), U64(0x0391AF45001FFF01), U64(0xD000900100290021),
		U64(0x2058480080040080), U64(0x6DCDFFA2002C38D0), U64(0xC709C80C00951002), U64(0xB70EE5420008FF84),
		U64(0x6E254003897FFCE6), U64(0xD91D21FE7E003901), U64(0xA0D1EFFF857FE001), U64(0x7C45FFC022001893),
		U64(0x8180818800800400), U64(0x2146001CB20018B0), U64(0x843C20E7DBFF8FEE), U64(0x09283C127A00083F),
		U64(0x01465F8CC0078000), U64(0xA30A50075FFD3FFF), U64(0x39593D8231FE0020), U64(0x8129FE58405E000F),
		U64(0x1140850008010011), U64(0x2302000830DA0044), U64(0xD706971819F400B0), U64(0xA0B2A3BC86E20004),
		U64(0x10FFF67AD3B88200), U64(0x10FFF67AD3B88200), U64(0x5076D15DBDF97E00), U64(0xD861C0D1FFC8DE00),
		U64(0x5CA002003B305E00), U64(0x84FFFFCF19605740), U64(0xD26F0FA80A28AC00), U64(0x342F7E87013BFA00),
		U64(0x63BB9E8FBF01FE7A), U64(0x260ADF40007B9101), U64(0x2013CEFF6000BEF7), U64(0x13AD6200060EBFE6),
		U64(0x2D4DFFFF28F4D9FA), U64(0x766200004B3A92F6), U64(0xB6AE6FF7FE8A070C), U64(0xD065F4839BFC4B02)
	};

	struct SquareData
	{
		Bitboard mask;
		u32 offset;
	};

	struct Data
	{
		std::array<SquareData, 64> data;
		u32 tableSize;
	};

	constexpr auto RookData = []
	{
		Data dst{};

		for (u32 i = 0; i < 64; ++i)
		{
			const auto square = static_cast<Square>(i);

			dst.data[i].mask = boards::All;

			for (const auto dir : {
				offsets::Up,
				offsets::Down,
				offsets::Left,
				offsets::Right
			})
			{
				const auto attacks = internal::generateSlidingAttacks(square, dir, 0);
				dst.data[i].mask &= ~(attacks & ~internal::edges(dir));
			}

			dst.data[i].offset = dst.tableSize;
			dst.tableSize += 1 << (64 - Shifts[i]);
		}

		return dst;
	}();
}
