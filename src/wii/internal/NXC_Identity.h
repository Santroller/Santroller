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

#ifndef NXC_Identity_h
#define NXC_Identity_h
#include <stdint.h>

enum class ExtensionType {
	NoController,
	AnyController,
	UnknownController,
	Nunchuk,
	ClassicController,
	GuitarController,
	DrumController,
	DJTurntableController,
};

namespace NintendoExtensionCtrl {
	inline ExtensionType identifyController(const uint8_t * idData) {
		if (idData[2] == 0xA4 && idData[3] == 0x20) {  // All valid IDs
			// Nunchuk ID: 0x0000
			if (idData[4] == 0x00 && idData[5] == 0x00) {
				return ExtensionType::Nunchuk;
			}

			// Classic Con. ID: 0x0101
			else if (idData[4] == 0x01 && idData[5] == 0x01) {
				return ExtensionType::ClassicController;
			}

			// Guitar Hero Controllers: 0x##00, 0xA420, 0x0103
			else if (idData[1] == 0x00
				&& idData[4] == 0x01 && idData[5] == 0x03) {

				// Guitar: 0x00
				if (idData[0] == 0x00) {
					return ExtensionType::GuitarController;
				}
				// Drums: 0x01
				else if (idData[0] == 0x01) {
					return ExtensionType::DrumController;
				}
				// DJ Turntable: 0x03
				else if (idData[0] == 0x03) {
					return ExtensionType::DJTurntableController;
				}
			}
		}

		return ExtensionType::UnknownController;  // No matches
	}
}

#endif
