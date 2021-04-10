#pragma once
#include "config/defines.h"
#include "controller/controller.h"
#include "eeprom/eeprom.h"
#include "i2c/i2c.h"
#include "pins/pins.h"
#include "util/util.h"
// #  include <avr/io.h>
#include "fxpt_math/fxpt_math.h"
#include "timer/timer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define I2C_ADDR 0x52
void tickWiiExtInput(Controller_t *controller);
// uint8_t wiiButtonBindings[16] = {
//     INVALID_PIN,  INVALID_PIN,    XBOX_START,     XBOX_HOME,
//     XBOX_BACK,    INVALID_PIN,    XBOX_DPAD_DOWN, XBOX_DPAD_RIGHT,
//     XBOX_DPAD_UP, XBOX_DPAD_LEFT, XBOX_RB,        XBOX_Y,
//     XBOX_A,       XBOX_X,         XBOX_B,         XBOX_LB};
// Inverted version of above
uint8_t wiiButtonBindings[XBOX_BTN_COUNT] = {
    8,  6,  9, 7,           2,  4,  INVALID_PIN, INVALID_PIN,
    15, 10, 3, INVALID_PIN, 12, 14, 13,          11};
uint16_t wiiExtensionID = WII_NO_EXTENSION;
uint16_t buttons;
uint8_t bytes = 6;
bool mapNunchukAccelToRightJoy;
void (*readFunction)(Controller_t *, uint8_t *) = NULL;

bool verifyData(const uint8_t *dataIn, uint8_t dataSize) {
  uint8_t orCheck = 0x00;  // Check if data is zeroed (bad connection)
  uint8_t andCheck = 0xFF; // Check if data is maxed (bad init)

  for (int i = 0; i < dataSize; i++) {
    orCheck |= dataIn[i];
    andCheck &= dataIn[i];
  }

  if (orCheck == 0x00 || andCheck == 0xFF) {
    return false; // No data or bad data
  }

  return true;
}

uint16_t readExtID(void) {
  uint8_t data[6];
  twi_readFromPointerSlow(I2C_ADDR, 0xFA, 6, data);
  if (!verifyData(data, sizeof(data))) { return WII_NOT_INITIALISED; }
  return data[0] << 8 | data[5];
}
void readDrumExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] - 0x20) << 10;
  controller->l_y = (data[1] - 0x20) << 10;
  // Mask out unused bits
  buttons = ~(data[4] | (data[5] << 8)) & 0xfeff;
  if (fullDeviceType >= MIDI_GAMEPAD && bit_check(data[3], 1)) {
    uint8_t vel = (7 - (data[3] >> 5)) << 5;
    uint8_t which = (data[2] & 0b01111100) >> 1;
    switch (which) {
    case 0x1B:
      drumVelocity[XBOX_RB - 8] = vel;
      break;
    case 0x19:
      drumVelocity[XBOX_B - 8] = vel;
      break;
    case 0x11:
      drumVelocity[XBOX_X - 8] = vel;
      break;
    case 0x0F:
      drumVelocity[XBOX_Y - 8] = vel;
      break;
    case 0x1E:
      drumVelocity[XBOX_LB - 8] = vel;
      break;
    case 0x12:
      drumVelocity[XBOX_A - 8] = vel;
      break;
    }
  }
  // The standard extension bindings are almost correct, but x and y are
  // swapped, so swap them
  bit_write(!bit_check(data[5], 3), buttons, wiiButtonBindings[XBOX_X]);
  bit_write(!bit_check(data[5], 5), buttons, wiiButtonBindings[XBOX_Y]);
}
void readGuitarExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = ((data[0] & 0x3f) - 32) << 10;
  controller->l_y = ((data[1] & 0x3f) - 32) << 10;
  // Whammy is weird, it ranges from 0 - 12. multiply by 2.5 to get from 0 - 36,
  // clamp, and then shift to 0 - 65535
  controller->r_x = ((data[3] & 0x1f) - 14);
  if (controller->r_x < 0) { controller->r_x = 0; }
  controller->r_x = (controller->r_x << 1) + controller->r_x;
  if (controller->r_x > 31) { controller->r_x = 31; }
  controller->r_x -= 16;
  controller->r_x <<= 11;

  buttons = ~(data[4] | data[5] << 8);
}
void readClassicExtHighRes(Controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] - 0x80) << 8;
  controller->l_y = (data[2] - 0x80) << 8;
  controller->r_x = (data[1] - 0x80) << 8;
  controller->r_y = (data[3] - 0x80) << 8;
  controller->lt = data[4];
  controller->rt = data[5];
  buttons = ~(data[6] | (data[7] << 8));
}
void readClassicExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] & 0x3f) - 32;
  controller->l_y = (data[1] & 0x3f) - 32;
  controller->r_x =
      (((data[0] & 0xc0) >> 3) | ((data[1] & 0xc0) >> 5) | (data[2] >> 7)) - 16;
  controller->r_y = (data[2] & 0x1f) - 16;
  controller->lt = ((data[3] >> 5) | ((data[2] & 0x60) >> 2));
  controller->rt = data[3] & 0x1f;
  buttons = ~(data[4] | data[5] << 8);
}
void readNunchukExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] - 0x80) << 8;
  controller->l_y = (data[1] - 0x80) << 8;
  if (mapNunchukAccelToRightJoy) {
    uint16_t accX = ((data[2] << 2) | ((data[5] & 0xC0) >> 6)) - 511;
    uint16_t accY = ((data[3] << 2) | ((data[5] & 0x30) >> 4)) - 511;
    uint16_t accZ = ((data[4] << 2) | ((data[5] & 0xC) >> 2)) - 511;
    controller->r_x = fxpt_atan2(accX, accZ);
    controller->r_y = fxpt_atan2(accY, accZ);
  }
  buttons = 0;
  bit_write(!bit_check(data[5], 0), buttons, wiiButtonBindings[XBOX_A]);
  bit_write(!bit_check(data[5], 1), buttons, wiiButtonBindings[XBOX_B]);
}
void readDJExt(Controller_t *controller, uint8_t *data) {
  uint8_t rtt =
      (data[2] & 0x80) >> 7 | (data[1] & 0xC0) >> 5 | (data[0] & 0xC0) >> 3;

  controller->l_x = ((data[0] & 0x3F) - 0x20) << 10;
  controller->l_y = ((data[1] & 0x3F) - 0x20) << 10;
  controller->r_x =
      (data[4] & 1) ? 32 + (0x1F - (data[3] & 0x1F)) : 32 - (data[3] & 0x1F);
  controller->r_y = (data[2] & 1) ? 32 + (0x1F - rtt) : 32 - rtt;
  controller->lt = (data[3] & 0xE0) >> 5 | (data[2] & 0x60) >> 2;
  controller->rt = (data[2] & 0x1E) >> 1;
  buttons = ~(data[4] << 8 | data[5]) & 0x63CD;
}
void readUDrawExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = ((data[2] & 0x0f) << 8) | data[0];
  controller->l_y = ((data[2] & 0xf0) << 4) | data[1];
  controller->rt = data[3];
  buttons = 0;
  bit_write(bit_check(data[5], 0), buttons, wiiButtonBindings[XBOX_A]);
  bit_write(bit_check(data[5], 1), buttons, wiiButtonBindings[XBOX_B]);
  bit_write(!bit_check(data[5], 2), buttons, wiiButtonBindings[XBOX_X]);
}
void readDrawsomeExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = data[0] | data[1] << 8;
  controller->l_y = data[2] | data[3] << 8;
  controller->rt = data[4] | (data[5] & 0x0f) << 8;
  // controller->status = data[5]>>4;
}
void readTataconExt(Controller_t *controller, uint8_t *data) {
  buttons = ~(data[4] << 8 | data[5]);
}
void initWiiExt(void) {
  wiiExtensionID = readExtID();
  if (wiiExtensionID == WII_NOT_INITIALISED) {
    // Send packets needed to initialise a controller
    twi_writeSingleToPointer(I2C_ADDR, 0xF0, 0x55);
    _delay_us(10);
    twi_writeSingleToPointer(I2C_ADDR, 0xFB, 0x00);
    _delay_us(10);
    wiiExtensionID = readExtID();
    _delay_us(10);
  }
  if (wiiExtensionID == WII_UBISOFT_DRAWSOME_TABLET) {
    twi_writeSingleToPointer(I2C_ADDR, 0xFB, 0x01);
    _delay_us(10);
    twi_writeSingleToPointer(I2C_ADDR, 0xF0, 0x55);
    _delay_us(10);
  }
  if (wiiExtensionID == WII_CLASSIC_CONTROLLER ||
      wiiExtensionID == WII_CLASSIC_CONTROLLER_PRO) {
    // Enable high-res mode
    twi_writeSingleToPointer(I2C_ADDR, 0xFE, 0x03);
    _delay_us(10);
    // Some controllers support high res mode, some dont. Some require it, some
    // dont. To mitigate this issue, we can check if the high res specific bytes
    // are zeroed. However this isnt enough. If a byte is corrupted during
    // transit than it may be triggered. Reading twice will allow us to confirm
    // that nothing was corrupted,
    uint8_t check[8];
    uint8_t validate[8];
    while (true) {
      twi_readFromPointerSlow(I2C_ADDR, 0, sizeof(check), check);
      _delay_us(200);
      twi_readFromPointerSlow(I2C_ADDR, 0, sizeof(validate), validate);
      if (memcmp(check, validate, sizeof(validate)) == 0) {
        bool highRes = (check[0x06] || check[0x07]);
        if (highRes) {
          readFunction = readClassicExtHighRes;
          bytes = 8;
        } else {
          readFunction = readClassicExt;
          bytes = 6;
        }
        break;
      }
      _delay_us(200);
    }
  }
  switch (wiiExtensionID) {
  case WII_GUITAR_HERO_GUITAR_CONTROLLER:
    readFunction = readGuitarExt;
    break;
  case WII_CLASSIC_CONTROLLER:
  case WII_CLASSIC_CONTROLLER_PRO:
    break;
  case WII_NUNCHUK:
    readFunction = readNunchukExt;
    break;
  case WII_GUITAR_HERO_DRUM_CONTROLLER:
    readFunction = readDrumExt;
    break;
  case WII_THQ_UDRAW_TABLET:
    readFunction = readUDrawExt;
    break;
  case WII_UBISOFT_DRAWSOME_TABLET:
    readFunction = readDrawsomeExt;
    break;
  case WII_DJ_HERO_TURNTABLE:
    readFunction = readDJExt;
    break;
  case WII_TAIKO_NO_TATSUJIN_CONTROLLER:
    readFunction = readTataconExt;
    break;
  default:
    wiiExtensionID = WII_NO_EXTENSION;
    readFunction = NULL;
  }
}
void tickWiiExtInput(Controller_t *controller) {
  uint8_t data[8];
  if (wiiExtensionID == WII_NOT_INITIALISED ||
      wiiExtensionID == WII_NO_EXTENSION ||
      (twi_readFromPointerSlow(I2C_ADDR, 0x00, bytes, data) &&
       !verifyData(data, bytes))) {
    initWiiExt();
    return;
  }
  if (readFunction) readFunction(controller, data);
}
bool readWiiButton(Pin_t pin) {
  uint8_t idx = wiiButtonBindings[pin.offset];
  if (idx == INVALID_PIN) return false;
  return !!bit_check(buttons, idx);
}
void initWiiExtensions(Configuration_t *config) {
  mapNunchukAccelToRightJoy = config->main.mapNunchukAccelToRightJoy;
}