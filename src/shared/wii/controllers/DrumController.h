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

#ifndef NXC_DrumController_h
#define NXC_DrumController_h

#include "../internal/ExtensionController.h"

#include "ClassicController.h"  // For joystick and +/- control maps

namespace NintendoExtensionCtrl {
	class DrumController_Shared : public ExtensionController {
	public:
		struct Maps {
			constexpr static ByteMap JoyX = ClassicController_Shared::Maps::LeftJoyX;
			constexpr static ByteMap JoyY = ClassicController_Shared::Maps::LeftJoyY;

			constexpr static BitMap  ButtonPlus = ClassicController_Shared::Maps::ButtonPlus;
			constexpr static BitMap  ButtonMinus = ClassicController_Shared::Maps::ButtonMinus;

			constexpr static BitMap  DrumRed = { 5, 6 };
			constexpr static BitMap  DrumBlue = { 5, 3 };
			constexpr static BitMap  DrumGreen = { 5, 4 };

			constexpr static BitMap  CymbalYellow = { 5, 5 };
			constexpr static BitMap  CymbalOrange = { 5, 7 };

			constexpr static BitMap  Pedal = { 5, 2 };

			constexpr static ByteMap Velocity = ByteMap(3, 3, 5, 5);
			constexpr static ByteMap VelocityID = ByteMap(2, 5, 1, 1);
			constexpr static BitMap  VelocityAvailable = { 2, 6 };
		};

		DrumController_Shared(ExtensionData &dataRef) :
			ExtensionController(dataRef, ExtensionType::DrumController) {}

		DrumController_Shared(ExtensionPort &port) :
			DrumController_Shared(port.getExtensionData()) {}

		enum VelocityID : uint8_t {
			None = 0x1F,
			Red = 0x19,
			Blue = 0x0F,
			Green = 0x12,
			Yellow = 0x11,
			Orange = 0x0E,
			Pedal = 0x1B,
		};

		uint8_t joyX() const;  // 6 bits, 0-63
		uint8_t joyY() const;

		boolean drumRed() const;
		boolean drumBlue() const;
		boolean drumGreen() const;

		boolean cymbalYellow() const;
		boolean cymbalOrange() const;

		boolean bassPedal() const;

		boolean buttonPlus() const;
		boolean buttonMinus() const;

		boolean velocityAvailable() const;
		VelocityID velocityID() const;

		uint8_t velocity() const;  // 3 bits, 0-7. 7 is fast/hard, 1 is slow/soft, 0 is not hit.
		uint8_t velocity(VelocityID idIn) const;  // velocity by ID

		uint8_t velocityRed() const;
		uint8_t velocityBlue() const;
		uint8_t velocityGreen() const;
		uint8_t velocityYellow() const;
		uint8_t velocityOrange() const;
		uint8_t velocityPedal() const;

		void printDebug() const;

	private:
		boolean validVelocityID(uint8_t idIn) const;
	};
}

using DrumController = NintendoExtensionCtrl::BuildControllerClass
	<NintendoExtensionCtrl::DrumController_Shared>;

#endif
