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

#include "DrumController.h"

namespace NintendoExtensionCtrl {

constexpr ByteMap DrumController_Shared::Maps::JoyX;
constexpr ByteMap DrumController_Shared::Maps::JoyY;

constexpr BitMap  DrumController_Shared::Maps::ButtonPlus;
constexpr BitMap  DrumController_Shared::Maps::ButtonMinus;

constexpr BitMap  DrumController_Shared::Maps::DrumRed;
constexpr BitMap  DrumController_Shared::Maps::DrumBlue;
constexpr BitMap  DrumController_Shared::Maps::DrumGreen;

constexpr BitMap  DrumController_Shared::Maps::CymbalYellow;
constexpr BitMap  DrumController_Shared::Maps::CymbalOrange;

constexpr BitMap  DrumController_Shared::Maps::Pedal;

constexpr ByteMap DrumController_Shared::Maps::Velocity;
constexpr ByteMap DrumController_Shared::Maps::VelocityID;
constexpr BitMap  DrumController_Shared::Maps::VelocityAvailable;

uint8_t DrumController_Shared::joyX() const {
	return getControlData(Maps::JoyX);
}

uint8_t DrumController_Shared::joyY() const {
	return getControlData(Maps::JoyY);
}

boolean DrumController_Shared::drumRed() const {
	return getControlBit(Maps::DrumRed);
}

boolean DrumController_Shared::drumBlue() const {
	return getControlBit(Maps::DrumBlue);
}

boolean DrumController_Shared::drumGreen() const {
	return getControlBit(Maps::DrumGreen);
}

boolean DrumController_Shared::cymbalYellow() const {
	return getControlBit(Maps::CymbalYellow);
}

boolean DrumController_Shared::cymbalOrange() const {
	return getControlBit(Maps::CymbalOrange);
}

boolean DrumController_Shared::bassPedal() const {
	return getControlBit(Maps::Pedal);
}

boolean DrumController_Shared::buttonPlus() const {
	return getControlBit(Maps::ButtonPlus);
}

boolean DrumController_Shared::buttonMinus() const {
	return getControlBit(Maps::ButtonMinus);
}

boolean DrumController_Shared::velocityAvailable() const {
	return getControlBit(Maps::VelocityAvailable);
}

DrumController_Shared::VelocityID DrumController_Shared::velocityID() const {
	uint8_t id = getControlData(Maps::VelocityID);  // 5 bit identifier

	if (validVelocityID(id)) {
		return (VelocityID) id;
	}

	return VelocityID::None;
}

boolean DrumController_Shared::validVelocityID(uint8_t idIn) const {
	switch (idIn) {
		case(VelocityID::None):  // Intentionally fall through cases
		case(VelocityID::Red):
		case(VelocityID::Blue):
		case(VelocityID::Green):
		case(VelocityID::Yellow):
		case(VelocityID::Orange):
		case(VelocityID::Pedal):
			return true;  // One of the above, is a valid ID
			break;
		default:
			return false;  // Not one of the above, invalid
	}
}

uint8_t DrumController_Shared::velocity() const {
	if (velocityAvailable()) {
		uint8_t velocityRaw = getControlData(Maps::Velocity);
		velocityRaw = 7 - velocityRaw;  // Invert so high = fast attack
		return velocityRaw;
	}
	return 0;  // Invalid data
}

uint8_t DrumController_Shared::velocity(VelocityID idIn) const {
	if (idIn == velocityID()) {
		return velocity();
	}
	return 0;  // ID mismatch
}

uint8_t DrumController_Shared::velocityRed() const {
	return velocity(VelocityID::Red);
}

uint8_t DrumController_Shared::velocityBlue() const {
	return velocity(VelocityID::Blue);
}

uint8_t DrumController_Shared::velocityGreen() const {
	return velocity(VelocityID::Green);
}

uint8_t DrumController_Shared::velocityYellow() const {
	return velocity(VelocityID::Yellow);
}

uint8_t DrumController_Shared::velocityOrange() const {
	return velocity(VelocityID::Orange);
}

uint8_t DrumController_Shared::velocityPedal() const {
	return velocity(VelocityID::Pedal);
}

}  // End "NintendoExtensionCtrl" namespace
