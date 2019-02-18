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

#ifndef NXC_Core_h
#define NXC_Core_h

#include "NXC_Comms.h"
namespace NintendoExtensionCtrl {

	// Utility
	boolean verifyData(const uint8_t * dataIn, uint8_t dataSize);

	class RolloverChange {
	public:
		RolloverChange(uint8_t min, uint8_t max);
		int8_t getChange(uint8_t valIn);
	private:
		int8_t rolloverOut(uint8_t c1, uint8_t c2) const;
		uint8_t halfRange() const;

		const uint8_t minValue;
		const uint8_t maxValue;

		uint8_t lastValue = 0;
	};
}

#endif
