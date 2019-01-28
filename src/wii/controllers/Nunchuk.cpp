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

#include "Nunchuk.h"

namespace NintendoExtensionCtrl {

constexpr CtrlIndex Nunchuk_Shared::Maps::JoyX;
constexpr CtrlIndex Nunchuk_Shared::Maps::JoyY;

constexpr CtrlIndex Nunchuk_Shared::Maps::AccelX_MSB;
constexpr ByteMap   Nunchuk_Shared::Maps::AccelX_LSB;

constexpr CtrlIndex Nunchuk_Shared::Maps::AccelY_MSB;
constexpr ByteMap   Nunchuk_Shared::Maps::AccelY_LSB;

constexpr CtrlIndex Nunchuk_Shared::Maps::AccelZ_MSB;
constexpr ByteMap   Nunchuk_Shared::Maps::AccelZ_LSB;

constexpr BitMap    Nunchuk_Shared::Maps::ButtonC;
constexpr BitMap    Nunchuk_Shared::Maps::ButtonZ;

uint8_t Nunchuk_Shared::joyX() const {
	return getControlData(Maps::JoyX);
}

uint8_t Nunchuk_Shared::joyY() const {
	return getControlData(Maps::JoyY);
}

uint16_t Nunchuk_Shared::accelX() const {
	return (getControlData(Maps::AccelX_MSB) << 2) | getControlData(Maps::AccelX_LSB);
}

uint16_t Nunchuk_Shared::accelY() const {
	return (getControlData(Maps::AccelY_MSB) << 2) | getControlData(Maps::AccelY_LSB);
}

uint16_t Nunchuk_Shared::accelZ() const {
	return (getControlData(Maps::AccelZ_MSB) << 2) | getControlData(Maps::AccelZ_LSB);
}

boolean Nunchuk_Shared::buttonC() const {
	return getControlBit(Maps::ButtonC);
}

boolean Nunchuk_Shared::buttonZ() const {
	return getControlBit(Maps::ButtonZ);
}

float Nunchuk_Shared::rollAngle() const {
	return atan2((float)accelX() - 511.0, (float)accelZ() - 511.0) * 180.0 / M_PI;
}

float Nunchuk_Shared::pitchAngle() const {
	// Inverted so pulling back is a positive pitch
	return -atan2((float)accelY() - 511.0, (float)accelZ() - 511.0) * 180.0 / M_PI;
}

}  // End "NintendoExtensionCtrl" namespace
