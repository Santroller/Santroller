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

#ifndef NXC_Comms_h
#define NXC_Comms_h

#include "../../twi/I2Cdev.h"
#include "NXC_Identity.h"
#include <stdlib.h>

namespace NintendoExtensionCtrl {
const long I2C_ConversionDelay = 175; // Microseconds, ~200 on AVR
const uint8_t I2C_Addr = 0x52;        // Address for all extension controllers

const uint8_t ID_Size = 6;

// Extension controller specific I2C functions
// -------------------------------------------
// Control Data
inline boolean initialize() {
  /* Initialization for unencrypted communication.
   * *Should* work on all devices, genuine + 3rd party.
   * See http://wiibrew.org/wiki/Wiimote/Extension_Controllers
   */
  I2Cdev::TWIWriteRegister(I2C_Addr, 0xF0, 0x55);
  I2Cdev::TWIWriteRegister(I2C_Addr, 0xFB, 0x00);
  return true;
}

inline boolean requestData(uint8_t ptr, size_t size, uint8_t *data) {
  return I2Cdev::readBytes(I2C_Addr, ptr, size, data);
}

inline boolean requestControlData(size_t size, uint8_t *controlData) {
  return I2Cdev::readBytes(I2C_Addr, 0x00, size, controlData);
}

// Identity
inline boolean requestIdentity(uint8_t *idData) {
  return I2Cdev::readBytes(I2C_Addr, 0xFA, ID_Size, idData);
}

inline ExtensionType identifyController() {
  uint8_t idData[ID_Size];

  if (!requestIdentity(idData)) {
    return ExtensionType::NoController; // Bad response from device
  }
  return identifyController(idData);
}
} // namespace NintendoExtensionCtrl

#endif
