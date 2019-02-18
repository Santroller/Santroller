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

#include "ClassicController.h"

namespace NintendoExtensionCtrl {

constexpr ByteMap ClassicController_Shared::Maps::LeftJoyX;
constexpr ByteMap ClassicController_Shared::Maps::LeftJoyY;

constexpr ByteMap ClassicController_Shared::Maps::RightJoyX[3];
constexpr ByteMap ClassicController_Shared::Maps::RightJoyY;

constexpr BitMap ClassicController_Shared::Maps::DpadUp;
constexpr BitMap ClassicController_Shared::Maps::DpadDown;
constexpr BitMap ClassicController_Shared::Maps::DpadLeft;
constexpr BitMap ClassicController_Shared::Maps::DpadRight;

constexpr BitMap ClassicController_Shared::Maps::ButtonA;
constexpr BitMap ClassicController_Shared::Maps::ButtonB;
constexpr BitMap ClassicController_Shared::Maps::ButtonX;
constexpr BitMap ClassicController_Shared::Maps::ButtonY;

constexpr ByteMap ClassicController_Shared::Maps::TriggerL[2];
constexpr ByteMap ClassicController_Shared::Maps::TriggerR;

constexpr BitMap ClassicController_Shared::Maps::ButtonL;
constexpr BitMap ClassicController_Shared::Maps::ButtonR;
constexpr BitMap ClassicController_Shared::Maps::ButtonZL;
constexpr BitMap ClassicController_Shared::Maps::ButtonZR;

constexpr BitMap ClassicController_Shared::Maps::ButtonPlus;
constexpr BitMap ClassicController_Shared::Maps::ButtonMinus;
constexpr BitMap ClassicController_Shared::Maps::ButtonHome;

uint8_t ClassicController_Shared::leftJoyX() const {
  return getControlData(Maps::LeftJoyX);
}

uint8_t ClassicController_Shared::leftJoyY() const {
  return getControlData(Maps::LeftJoyY);
}

uint8_t ClassicController_Shared::rightJoyX() const {
  return getControlData(Maps::RightJoyX);
}

uint8_t ClassicController_Shared::rightJoyY() const {
  return getControlData(Maps::RightJoyY);
}

boolean ClassicController_Shared::dpadUp() const {
  return getControlBit(Maps::DpadUp);
}

boolean ClassicController_Shared::dpadDown() const {
  return getControlBit(Maps::DpadDown);
}

boolean ClassicController_Shared::dpadLeft() const {
  return getControlBit(Maps::DpadLeft);
}

boolean ClassicController_Shared::dpadRight() const {
  return getControlBit(Maps::DpadRight);
}

boolean ClassicController_Shared::buttonA() const {
  return getControlBit(Maps::ButtonA);
}

boolean ClassicController_Shared::buttonB() const {
  return getControlBit(Maps::ButtonB);
}

boolean ClassicController_Shared::buttonX() const {
  return getControlBit(Maps::ButtonX);
}

boolean ClassicController_Shared::buttonY() const {
  return getControlBit(Maps::ButtonY);
}

uint8_t ClassicController_Shared::triggerL() const {
  return getControlData(Maps::TriggerL);
}

uint8_t ClassicController_Shared::triggerR() const {
  return getControlData(Maps::TriggerR);
}

boolean ClassicController_Shared::buttonL() const {
  return getControlBit(Maps::ButtonL);
}

boolean ClassicController_Shared::buttonR() const {
  return getControlBit(Maps::ButtonR);
}

boolean ClassicController_Shared::buttonZL() const {
  return getControlBit(Maps::ButtonZL);
}

boolean ClassicController_Shared::buttonZR() const {
  return getControlBit(Maps::ButtonZR);
}

boolean ClassicController_Shared::buttonStart() const { return buttonPlus(); }

boolean ClassicController_Shared::buttonSelect() const { return buttonMinus(); }

boolean ClassicController_Shared::buttonPlus() const {
  return getControlBit(Maps::ButtonPlus);
}

boolean ClassicController_Shared::buttonMinus() const {
  return getControlBit(Maps::ButtonMinus);
}

boolean ClassicController_Shared::buttonHome() const {
  return getControlBit(Maps::ButtonHome);
}

} // namespace NintendoExtensionCtrl
