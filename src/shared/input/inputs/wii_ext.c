#include "wii_ext.h"
#include "config/defines.h"
#include "config/eeprom.h"
#include "util/util.h"
#include "i2c/i2c.h"
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
uint8_t wiiExtButtonBindings[16] = {
    INVALID_PIN,  INVALID_PIN,    XBOX_START,     XBOX_HOME,
    XBOX_BACK,    INVALID_PIN,    XBOX_DPAD_DOWN, XBOX_DPAD_RIGHT,
    XBOX_DPAD_UP, XBOX_DPAD_LEFT, XBOX_RB,        XBOX_Y,
    XBOX_A,       XBOX_X,         XBOX_B,         XBOX_LB};
uint16_t counter;
uint16_t wii_ext = WII_NO_DEVICE;
void (*readFunction)(Controller_t *, uint8_t *) = NULL;

void readWiiExtButtons(Controller_t *controller, uint16_t buttons) {
  for (uint8_t i = 0; i < sizeof(wiiExtButtonBindings); i++) {
    uint8_t idx = wiiExtButtonBindings[i];
    if (idx == INVALID_PIN) continue;
    bit_write(bit_check(buttons, i), controller->buttons, idx);
  }
}

uint16_t readWiiExtID(void) {
  uint8_t data[6];
  if (readFromI2CPointerSlow(I2C_ADDR, 0xFA, 6, data)) {
    return WII_NO_DEVICE;
  }
  // 0100 a420 0101 -> #1#######101
  // 0100 a420 0103 -> #1#######101
  return (data[0] & 0xf) << 12 | (data[4] & 0xf) << 8 | data[5];
}

void readDrumExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] - 0x20) << 10;
  controller->l_y = (data[1] - 0x20) << 10;
  // Mask out unused bits
  uint16_t buttons = ~(data[4] | (data[5] << 8)) & 0xfeff;
  readWiiExtButtons(controller, buttons);
  if (config.main.subType >= MIDI_GUITAR && bit_check(data[3], 1)) {
    uint8_t vel = (7 - (data[3] >> 5)) << 5;
    uint8_t which = (data[2] & 0b01111100) >> 1;
    switch (which) {
    case 0x1B:
      controller->drumVelocity[XBOX_RB - 8] = vel;
      break;
    case 0x19:
      controller->drumVelocity[XBOX_B - 8] = vel;
      break;
    case 0x11:
      controller->drumVelocity[XBOX_X - 8] = vel;
      break;
    case 0x0F:
      controller->drumVelocity[XBOX_Y - 8] = vel;
      break;
    case 0x1E:
      controller->drumVelocity[XBOX_LB - 8] = vel;
      break;
    case 0x12:
      controller->drumVelocity[XBOX_A - 8] = vel;
      break;
    }
  }
  // The standard extension bindings are almost correct, but x and y are swapped, so swap them
  bit_write(!bit_check(data[5], 3), controller->buttons, XBOX_X);
  bit_write(!bit_check(data[5], 5), controller->buttons, XBOX_Y);
}
void readGuitarExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = ((data[0] & 0x3f) - 32) << 10;
  controller->l_y = ((data[1] & 0x3f) - 32) << 10;
  controller->r_x = -(((data[3] & 0x1f) - 16) << 10);
  uint16_t buttons = ~(data[4] | data[5] << 8);
  readWiiExtButtons(controller, buttons);
}
void readClassicExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] - 0x80) << 8;
  controller->l_y = (data[2] - 0x80) << 8;
  controller->r_x = (data[1] - 0x80) << 8;
  controller->r_y = (data[3] - 0x80) << 8;
  controller->lt = data[4];
  controller->rt = data[5];
  uint16_t buttons = ~(data[6] | (data[7] << 8));
  readWiiExtButtons(controller, buttons);
}
void readNunchukExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] - 0x80) << 8;
  controller->l_y = (data[2] - 0x80) << 8;
  if (config.main.mapNunchukAccelToRightJoy) {
    uint16_t accX = (data[2] << 2) | ((data[5] & 0xC0) >> 6);
    uint16_t accY = (data[3] << 2) | ((data[5] & 0x30) >> 4);
    uint16_t accZ = (data[4] << 2) | ((data[5] & 0xC) >> 2);
    controller->r_x =
        atan2((float)accX - 511.0, (float)accZ - 511.0) * 180.0 / M_PI;
    controller->r_y =
        -atan2((float)accY - 511.0, (float)accZ - 511.0) * 180.0 / M_PI;
  }
  bit_write(!bit_check(data[5], 0), controller->buttons, XBOX_A);
  bit_write(!bit_check(data[5], 1), controller->buttons, XBOX_B);
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
  uint16_t buttons = ~(data[4] << 8 | data[5]) & 0x63CD;
  readWiiExtButtons(controller, buttons);
}
void readUDrawExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = ((data[2] & 0x0f) << 8) | data[0];
  controller->l_y = ((data[2] & 0xf0) << 4) | data[1];
  controller->rt = data[3];
  bit_write(bit_check(data[5], 0), controller->buttons, XBOX_A);
  bit_write(bit_check(data[5], 1), controller->buttons, XBOX_B);
  bit_write(!bit_check(data[5], 2), controller->buttons, XBOX_X);
}
void readDrawsomeExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = data[0] | data[1] << 8;
  controller->l_y = data[2] | data[3] << 8;
  controller->rt = data[4] | (data[5] & 0x0f) << 8;
  // controller->status = data[5]>>4;
}
void readTataconTick(Controller_t *controller, uint8_t *data) {
  uint16_t buttons = ~(data[4] << 8 | data[5]);
  readWiiExtButtons(controller, buttons);
}
void initWiiExt(void) {
  wii_ext = readWiiExtID();
  if (wii_ext == WII_NO_DEVICE) {
    _delay_us(10);
    writeSingleToI2CPointer(I2C_ADDR, 0xF0, 0x55);
    _delay_us(10);
    writeSingleToI2CPointer(I2C_ADDR, 0xFB, 0x00);
    _delay_us(10);
  }
  wii_ext = readWiiExtID();
  if (wii_ext == WII_CLASSIC_CONTROLLER ||
      wii_ext == WII_CLASSIC_CONTROLLER_PRO) {
    // Enable high-res mode
    writeSingleToI2CPointer(I2C_ADDR, 0xFE, 0x03);
    _delay_us(10);
  }
  switch (wii_ext) {
  case WII_GUITAR_HERO_GUITAR_CONTROLLER:
    readFunction = readGuitarExt;
    break;
  case WII_CLASSIC_CONTROLLER:
  case WII_CLASSIC_CONTROLLER_PRO:
    readFunction = readClassicExt;
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
    readFunction = readTataconTick;
    break;
  default:
    readFunction = NULL;
  }
}
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
void tickWiiExtInput(Controller_t *controller) {
  uint8_t data[8];
  if (wii_ext == WII_NO_DEVICE ||
      readFromI2CPointerSlow(I2C_ADDR, 0x00, sizeof(data), data) ||
      !verifyData(data, sizeof(data))) {
    initWiiExt();
    return;
  }
  if (readFunction) readFunction(controller, data);
}
