#pragma once
#include "config/defines.h"
#include "controller/controller.h"
#include "eeprom/eeprom.h"
#include "fxpt_math/fxpt_math.h"
#include "i2c/i2c.h"
#include "pins/pins.h"
#include "timer/timer.h"
#include "util/util.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define I2C_ADDR 0x52
#define READ_ID 0xFA
#define ID_LEN 6
#define SET_RES_MODE 0xFE
#define LOWRES_MODE 0x03
#define HIGHRES_MODE 0x03
void tickWiiExtInput(Controller_t *controller);
uint8_t wiiButtonBindings[XBOX_BTN_COUNT] = {[XBOX_DPAD_UP] = WII_DPAD_UP,
                                             [XBOX_DPAD_DOWN] = WII_DPAD_DOWN,
                                             [XBOX_DPAD_LEFT] = WII_DPAD_LEFT,
                                             [XBOX_DPAD_RIGHT] = WII_DPAD_RIGHT,
                                             [XBOX_START] = WII_PLUS,
                                             [XBOX_BACK] = WII_MINUS,
                                             [XBOX_LEFT_STICK] = INVALID_PIN,
                                             [XBOX_RIGHT_STICK] = INVALID_PIN,
                                             [XBOX_LB] = WII_ZL,
                                             [XBOX_RB] = WII_ZR,
                                             [XBOX_HOME] = WII_HOME,
                                             [XBOX_UNUSED] = INVALID_PIN,
                                             [XBOX_A] = WII_A,
                                             [XBOX_B] = WII_B,
                                             [XBOX_X] = WII_Y,
                                             [XBOX_Y] = WII_X};
uint16_t wiiExtensionID = WII_NO_EXTENSION;
uint16_t buttons;
uint8_t bytes = 6;
uint8_t dataReadIndex = 0;
bool guitarTapBar = false;
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
  uint8_t data[ID_LEN];
  memset(data, 0, sizeof(data));
  twi_readFromPointerSlow(I2C_ADDR, READ_ID, ID_LEN, data);
  _delay_us(200);
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
    default:
      break;
    }
  }
  // The standard extension bindings are almost correct, but x and y are
  // swapped, so swap them
  bit_write(!bit_check(data[5], 3), buttons, wiiButtonBindings[XBOX_X]);
  bit_write(!bit_check(data[5], 5), buttons, wiiButtonBindings[XBOX_Y]);
}
uint8_t wiiwttapbindings[] = {[0x2] = (_BV(XBOX_A)) >> 8,
                              [0x3] = (_BV(XBOX_A) | _BV(XBOX_B)) >> 8,
                              [0x5] = (_BV(XBOX_B)) >> 8,
                              [0x6] = (_BV(XBOX_B) | _BV(XBOX_X)) >> 8,
                              [0x9] = (_BV(XBOX_Y)) >> 8,
                              [0xa] = (_BV(XBOX_X) | _BV(XBOX_Y)) >> 8,
                              [0xb] = (_BV(XBOX_X)) >> 8,
                              [0xc] = (_BV(XBOX_X)) >> 8,
                              [0xd] = (_BV(XBOX_X) | _BV(XBOX_LB)) >> 8,
                              [0xf] = (_BV(XBOX_LB)) >> 8};

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
  if (guitarTapBar) {
    uint8_t tap = ((data[3] & 0x1f) - 14);
    buttons |= wiiwttapbindings[tap >> 1] << 8;
  }
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
      ((((data[0] & 0xc0) >> 3) | ((data[1] & 0xc0) >> 5) | (data[2] >> 7)) -
       16)
      << 10;
  controller->r_y = ((data[2] & 0x1f) - 16) << 10;
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

  uint8_t ltt =
      (data[4] & 1) ? 32 + (0x1F - (data[3] & 0x1F)) : 32 - (data[3] & 0x1F);
  rtt = (data[2] & 1) ? 32 + (0x1F - rtt) : 32 - rtt;
  uint8_t effect_dial = (data[3] & 0xE0) >> 5 | (data[2] & 0x60) >> 2;
  uint8_t crossfade = (data[2] & 0x1E) >> 1;

  controller->l_x = ltt;
  controller->l_y = rtt;
  controller->r_x = effect_dial;
  controller->r_y = crossfade;
  
  buttons = ~(data[4] << 8 | data[5]) & 0x63CD;
  
  int8_t l_x = ((data[0] & 0x3F) - 0x20);
  int8_t l_y = ((data[1] & 0x3F) - 0x20);
  if (l_x < -32) { bit_set(controller->buttons, XBOX_DPAD_LEFT); }
  if (l_x > 32) { bit_set(controller->buttons, XBOX_DPAD_RIGHT); }
  if (l_y < -32) { bit_set(controller->buttons, XBOX_DPAD_UP); }
  if (l_y > 32) { bit_set(controller->buttons, XBOX_DPAD_DOWN); }
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
  // We can just skip all the other bytes except for the buttons
  buttons = ~(data[0]);
}
void initWiiExt(void) {
  // twi_init(false);
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
    // Drawsome tablet needs some additional init
    twi_writeSingleToPointer(I2C_ADDR, 0xFB, 0x01);
    _delay_us(10);
    twi_writeSingleToPointer(I2C_ADDR, 0xF0, 0x55);
    _delay_us(10);
  }
  dataReadIndex = 0;
  if (wiiExtensionID == WII_GUITAR_HERO_GUITAR_CONTROLLER) {
    readFunction = readGuitarExt;
  } else if (wiiExtensionID == WII_CLASSIC_CONTROLLER ||
             wiiExtensionID == WII_CLASSIC_CONTROLLER_PRO) {
    // We know that classic controllers and classic pro controllers support
    // higher speed
    // twi_init(false);
    // Enable high-res mode (try a few times, sometimes the controller doesnt
    // pick it up)
    for (int i = 0; i < 3; i++) {
      twi_writeSingleToPointer(I2C_ADDR, SET_RES_MODE, HIGHRES_MODE);
      _delay_us(200);
    }

    // Some controllers support high res mode, some dont. Some require it, some
    // dont. When a controller goes into high res mode, its ID will change,
    // so check.

    uint8_t id[ID_LEN];
    twi_readFromPointerSlow(I2C_ADDR, READ_ID, ID_LEN, id);
    _delay_us(200);
    if (id[4] == HIGHRES_MODE) {
      readFunction = readClassicExtHighRes;
      bytes = 8;
    } else {
      readFunction = readClassicExt;
      bytes = 6;
    }
  } else if (wiiExtensionID == WII_NUNCHUK) {
    readFunction = readNunchukExt;
  } else if (wiiExtensionID == WII_GUITAR_HERO_DRUM_CONTROLLER) {
    readFunction = readDrumExt;
  } else if (wiiExtensionID == WII_THQ_UDRAW_TABLET) {
    readFunction = readUDrawExt;
  } else if (wiiExtensionID == WII_UBISOFT_DRAWSOME_TABLET) {
    readFunction = readDrawsomeExt;
  } else if (wiiExtensionID == WII_DJ_HERO_TURNTABLE) {
    readFunction = readDJExt;
  } else if (wiiExtensionID == WII_TAIKO_NO_TATSUJIN_CONTROLLER) {
    // We can cheat a little with these controllers, as most of the bytes that
    // get read back are constant. Hence we start at 0x5 instead of 0x0.
    readFunction = readTataconExt;
    dataReadIndex = 5;
    bytes = 1;
  } else {
    wiiExtensionID = WII_NO_EXTENSION;
    readFunction = NULL;
  }
}
void tickWiiExtInput(Controller_t *controller) {
  uint8_t data[8];
  memset(data, 0, sizeof(data));
  if (wiiExtensionID == WII_NOT_INITIALISED ||
      wiiExtensionID == WII_NO_EXTENSION ||
      !twi_readFromPointerSlow(I2C_ADDR, dataReadIndex, bytes, data) ||
      !verifyData(data, bytes)) {
    initWiiExt();
    return;
  }
  if (readFunction) readFunction(controller, data);
}
bool readWiiButton(Pin_t *pin) {
  uint8_t idx = wiiButtonBindings[pin->offset];
  if (idx == INVALID_PIN) return false;
  return !!bit_check(buttons, idx);
}
void initWiiExtensions(Configuration_t *config) {
  mapNunchukAccelToRightJoy = config->main.mapNunchukAccelToRightJoy;
  guitarTapBar = config->neck.wiiNeck;
}