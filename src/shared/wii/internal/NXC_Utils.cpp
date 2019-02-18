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

#include "NXC_Utils.h"

namespace NintendoExtensionCtrl {

boolean verifyData(const uint8_t *dataIn, uint8_t dataSize) {
  byte orCheck = 0x00;  // Check if data is zeroed (bad connection)
  byte andCheck = 0xFF; // Check if data is maxed (bad init)

  for (int i = 0; i < dataSize; i++) {
    orCheck |= dataIn[i];
    andCheck &= dataIn[i];
  }

  if (orCheck == 0x00 || andCheck == 0xFF) {
    return false; // No data or bad data
  }

  return true;
}

RolloverChange::RolloverChange(uint8_t min, uint8_t max)
    : minValue(min), maxValue(max) {}

int8_t RolloverChange::getChange(uint8_t valIn) {
  if (valIn == lastValue || valIn < minValue || valIn > maxValue) {
    return 0; // No change, or out of range
  }

  int8_t outValue = 0;
  if (abs(valIn - lastValue) >= halfRange()) { // Assume rollover
    // Rollover, going up
    if (valIn < lastValue) {
      outValue = rolloverOut(valIn, lastValue);
    }
    // Rollover, going down
    else if (valIn > lastValue) {
      outValue = -rolloverOut(lastValue, valIn);
    }
  } else { // Normal change
    outValue = valIn - lastValue;
  }

  lastValue = valIn; // Store current value for comparison next time
  return outValue;
}

int8_t RolloverChange::rolloverOut(uint8_t c1, uint8_t c2) const {
  return (maxValue - c2) + c1 + 1;
}

uint8_t RolloverChange::halfRange() const {
  return ((maxValue - minValue) / 2) + 1;
}
} // namespace NintendoExtensionCtrl
