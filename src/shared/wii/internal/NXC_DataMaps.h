/*
*  Project     Nintendo Extension Controller Library
*  @author     David Madison
*  @link       github.com/dmadison/NintendoExtensionCtrl
*  @license    LGPLv3 - Copyright (c) 2018 David Madison
*
*  This file is part of the Nintendo Extension Controller Library.
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Lesser General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NXC_DataMaps_h
#define NXC_DataMaps_h
#include <stdint.h>

namespace NintendoExtensionCtrl {
	// Map data alias for single byte data, using the full byte
	using CtrlIndex = uint8_t;

	// Map data struct for single *byte* data, using a portion of a byte
	struct ByteMap {
		constexpr ByteMap(
			uint8_t index,     // Index in the control data array
			uint8_t size,      // Size of the data block, in bits
			uint8_t position,  // Start position of the data block, in bits from right
			uint8_t offset)    // Amount to shift the final data to the right
			: index(index), mask(BuildMask(size <= 8 ? size : 8, position)), offset(offset) {}

		const uint8_t index;
		const uint8_t mask;  // Data mask for bitwise operations, formed from size and start position
		const uint8_t offset;

		constexpr static uint8_t BuildMask(uint8_t size, uint8_t startPos) {
			return (0xFF >> (8 - size)) << startPos;  // Bitmask, to size and position of the data
		}
	};

	// Map data struct for single *bit* data (with inversion)
	struct BitMap {
		const uint8_t index;     // Index in the control data array
		const uint8_t position;  // Position of the bit, from right
	};
}

#endif
