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

#include "DJTurntable.h"

namespace NintendoExtensionCtrl {

constexpr ByteMap DJTurntableController_Shared::Maps::JoyX;
constexpr ByteMap DJTurntableController_Shared::Maps::JoyY;

constexpr BitMap  DJTurntableController_Shared::Maps::ButtonPlus;
constexpr BitMap  DJTurntableController_Shared::Maps::ButtonMinus;

constexpr ByteMap DJTurntableController_Shared::Maps::Left_Turntable;
constexpr ByteMap DJTurntableController_Shared::Maps::Left_TurntableSign;
constexpr BitMap  DJTurntableController_Shared::Maps::Left_ButtonGreen;
constexpr BitMap  DJTurntableController_Shared::Maps::Left_ButtonRed;
constexpr BitMap  DJTurntableController_Shared::Maps::Left_ButtonBlue;

constexpr ByteMap DJTurntableController_Shared::Maps::Right_Turntable[3];
constexpr ByteMap DJTurntableController_Shared::Maps::Right_TurntableSign;
constexpr BitMap  DJTurntableController_Shared::Maps::Right_ButtonGreen;
constexpr BitMap  DJTurntableController_Shared::Maps::Right_ButtonRed;
constexpr BitMap  DJTurntableController_Shared::Maps::Right_ButtonBlue;

constexpr ByteMap DJTurntableController_Shared::Maps::EffectDial[2];
constexpr ByteMap DJTurntableController_Shared::Maps::CrossfadeSlider;

constexpr BitMap  DJTurntableController_Shared::Maps::ButtonEuphoria;

// Combined Turntable
int8_t DJTurntableController_Shared::turntable() const {
	return left.turntable() + right.turntable();
}

boolean DJTurntableController_Shared::buttonGreen() const {
	return left.buttonGreen() | right.buttonGreen();
}

boolean DJTurntableController_Shared::buttonRed() const {
	return left.buttonRed() | right.buttonRed();
}

boolean DJTurntableController_Shared::buttonBlue() const {
	return left.buttonBlue() | right.buttonBlue();
}

// Main Board
uint8_t DJTurntableController_Shared::effectDial() const {
	return getControlData(Maps::EffectDial);
}

int8_t DJTurntableController_Shared::crossfadeSlider() const {
	return getControlData(Maps::CrossfadeSlider) - 8;  // Shifted to signed int
}

boolean DJTurntableController_Shared::buttonEuphoria() const {
	return getControlBit(Maps::ButtonEuphoria);
}

uint8_t DJTurntableController_Shared::joyX() const {
	return getControlData(Maps::JoyX);
}

uint8_t DJTurntableController_Shared::joyY() const {
	return getControlData(Maps::JoyY);
}

boolean DJTurntableController_Shared::buttonPlus() const {
	return getControlBit(Maps::ButtonPlus);
}

boolean DJTurntableController_Shared::buttonMinus() const {
	return getControlBit(Maps::ButtonMinus);
}

DJTurntableController_Shared::TurntableConfig DJTurntableController_Shared::getTurntableConfig() {
	if (tableConfig == TurntableConfig::Both) {
		return tableConfig;  // Both are attached, no reason to check data
	}

	boolean leftState = left.connected();
	boolean rightState = right.connected();

	if (leftState && rightState) {
		return tableConfig = TurntableConfig::Both;
	}
	else if (leftState) {
		return tableConfig = TurntableConfig::Left;
	}
	else if (rightState) {
		return tableConfig = TurntableConfig::Right;
	}
	else {
		return tableConfig = TurntableConfig::BaseOnly;
	}
}

uint8_t DJTurntableController_Shared::getNumTurntables() {
	getTurntableConfig();  // Update config from data

	switch (tableConfig) {
		case TurntableConfig::BaseOnly:
			return 0;
			break;
		case TurntableConfig::Left:
		case TurntableConfig::Right:
			return 1;
			break;
		case TurntableConfig::Both:
			return 2;
			break;
	}
	return 0;  // Just in-case
}


// Turntable Expansion Base
boolean DJTurntableController_Shared::TurntableExpansion::connected() const {
	if (base.tableConfig == TurntableConfig::Both || base.tableConfig == side) {
		return true;  // Already checked
	}
	return turntable() != 0 || buttonGreen() || buttonRed() || buttonBlue();
}

// Left Turntable
int8_t DJTurntableController_Shared::TurntableLeft::turntable() const {
	uint8_t turnData = base.getControlData(Maps::Left_Turntable);
	boolean turnSign = base.getControlData(Maps::Left_TurntableSign);
	return getTurntableSpeed(turnData, turnSign);
}

boolean DJTurntableController_Shared::TurntableLeft::buttonGreen() const {
	return base.getControlBit(Maps::Left_ButtonGreen);
}

boolean DJTurntableController_Shared::TurntableLeft::buttonRed() const {
	return base.getControlBit(Maps::Left_ButtonRed);
}

boolean DJTurntableController_Shared::TurntableLeft::buttonBlue() const {
	return base.getControlBit(Maps::Left_ButtonBlue);
}

// Right Turntable
int8_t DJTurntableController_Shared::TurntableRight::turntable() const {
	uint8_t turnData = base.getControlData(Maps::Right_Turntable);
	boolean turnSign = base.getControlData(Maps::Right_TurntableSign);
	return getTurntableSpeed(turnData, turnSign);
}

boolean DJTurntableController_Shared::TurntableRight::buttonGreen() const {
	return base.getControlBit(Maps::Right_ButtonGreen);
}

boolean DJTurntableController_Shared::TurntableRight::buttonRed() const {
	return base.getControlBit(Maps::Right_ButtonRed);
}

boolean DJTurntableController_Shared::TurntableRight::buttonBlue() const {
	return base.getControlBit(Maps::Right_ButtonBlue);
}

// Effect Rollover
int8_t DJTurntableController_Shared::EffectRollover::getChange() {
	return RolloverChange::getChange(dj.effectDial());
}

}  // End "NintendoExtensionCtrl" namespace

