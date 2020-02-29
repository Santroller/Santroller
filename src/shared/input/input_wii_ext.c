#include "input_wii_ext.h"
#include "../config/defines.h"
#include "../config/eeprom.h"
#include "../util.h"
#include "i2c/twi.h"
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
uint8_t classic_bindings[16] = {
    INVALID_PIN,  INVALID_PIN,    XBOX_START,     XBOX_HOME,
    XBOX_BACK,    INVALID_PIN,    XBOX_DPAD_DOWN, XBOX_DPAD_RIGHT,
    XBOX_DPAD_UP, XBOX_DPAD_LEFT, XBOX_RB,        XBOX_Y,
    XBOX_A,       XBOX_X,         XBOX_B,         XBOX_LB};
uint16_t counter;
uint16_t id = NO_DEVICE;
void (*readFunction)(controller_t *, uint8_t *) = NULL;

void read_buttons(controller_t *controller, uint16_t buttons) {
  for (uint8_t i = 0; i < sizeof(classic_bindings); i++) {
    uint8_t idx = classic_bindings[i];
    if (idx == INVALID_PIN) continue;
    bit_write(bit_check(buttons, i), controller->buttons, idx);
  }
}

uint16_t read_ext_id(void) {
  uint8_t data[6];
  if (twi_readFromPointerSlow(I2C_ADDR, 0xFA, 6, data)) { return NO_DEVICE; }
  // 0100 a420 0101 -> #1#######101
  // 0100 a420 0103 -> #1#######101
  return (data[0] & 0xf) << 12 | (data[4] & 0xf) << 8 | data[5];
}
void write_byte(uint8_t addr, uint8_t data) {
  twi_writeToPointer(I2C_ADDR, addr, 1, &data);
}
void drum_tick(controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] - 0x20) << 10;
  controller->l_y = (data[1] - 0x20) << 10;
  // Mask out unused bits
  uint16_t buttons = ~(data[4] | (data[5] << 8)) & 0xfeff;
  read_buttons(controller, buttons);
}
void guitar_cnt_tick(controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] - 0x80) << 8;
  controller->l_y = (data[2] - 0x80) << 8;
  int32_t whammy = (data[5] - 0x80) << 8;
  controller->r_x = constrain(whammy, 0, 32767);
  uint16_t buttons = ~(data[6] | (data[7] << 8));
  read_buttons(controller, buttons);
}
void classic_tick(controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] - 0x80) << 8;
  controller->l_y = (data[2] - 0x80) << 8;
  controller->r_x = (data[1] - 0x80) << 8;
  controller->r_y = (data[3] - 0x80) << 8;
  controller->lt = data[4];
  controller->rt = data[5];
  uint16_t buttons = ~(data[6] | (data[7] << 8));
  read_buttons(controller, buttons);
}
void nunchuk_tick(controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] - 0x80) << 8;
  controller->l_y = (data[2] - 0x80) << 8;
  if (config.main.map_accel_to_right) {
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
void dj_tick(controller_t *controller, uint8_t *data) {
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
  read_buttons(controller, buttons);
}
void udraw_tick(controller_t *controller, uint8_t *data) {
  controller->l_x = ((data[2] & 0x0f) << 8) | data[0];
  controller->l_y = ((data[2] & 0xf0) << 4) | data[1];
  controller->rt = data[3];
  bit_write(bit_check(data[5], 0), controller->buttons, XBOX_A);
  bit_write(bit_check(data[5], 1), controller->buttons, XBOX_B);
  bit_write(!bit_check(data[5], 2), controller->buttons, XBOX_X);
}
void drawsome_tick(controller_t *controller, uint8_t *data) {
  controller->l_x = data[0] | data[1] << 8;
  controller->l_y = data[2] | data[3] << 8;
  controller->rt = data[4] | (data[5] & 0x0f) << 8;
  // controller->status = data[5]>>4;
}
void tatacon_tick(controller_t *controller, uint8_t *data) {
  uint16_t buttons = ~(data[4] << 8 | data[5]);
  read_buttons(controller, buttons);
}
void init_controller(void) {
  _delay_us(10);
  write_byte(0xF0, 0x55);
  _delay_us(10);
  write_byte(0xFB, 0x00);
  _delay_us(10);
  id = read_ext_id();
  if (id == GUITAR || id == CLASSIC || id == CLASSIC_PRO) {
    // Enable high-res mode
    write_byte(0xFE, 0x03);
    _delay_us(10);
  }
  switch (id) {
  case GUITAR:
    readFunction = guitar_cnt_tick;
    break;
  case CLASSIC:
  case CLASSIC_PRO:
    readFunction = classic_tick;
    break;
  case NUNCHUK:
    readFunction = nunchuk_tick;
    break;
  case DRUMS:
    readFunction = drum_tick;
    break;
  case UDRAW:
    readFunction = udraw_tick;
    break;
  case DRAWSOME:
    readFunction = drawsome_tick;
    break;
  case TURNTABLE:
    readFunction = dj_tick;
    break;
  case TATACON:
    readFunction = tatacon_tick;
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
void wii_ext_tick(controller_t *controller) {
  uint8_t data[8];
  if (twi_readFromPointerSlow(I2C_ADDR, 0x00, sizeof(data), data) ||
      !verifyData(data, sizeof(data))) {
    init_controller();
    return;
  }
  readFunction(controller, data);
}

void get_wii_device_name(char *str) {
  switch (id) {
  case NUNCHUK:
    strcpy(str, "Nunchuk");
    break;
  case CLASSIC:
    strcpy(str, "Classic Controller");
    break;
  case CLASSIC_PRO:
    strcpy(str, "Classic Controller Pro");
    break;
  case UDRAW:
    strcpy(str, "THQ uDraw Tablet");
    break;
  case DRAWSOME:
    strcpy(str, "Ubisoft Drawsome Tablet");
    break;
  case GUITAR:
    strcpy(str, "Guitar Hero Guitar Controller");
    break;
  case DRUMS:
    strcpy(str, "Guitar Hero Drum Controller");
    break;
  case TURNTABLE:
    strcpy(str, "DJ Hero Turntable");
    break;
  case TATACON:
    strcpy(str, "Taiko no Tatsujin controller");
    break;
  case MOTION_PLUS:
    strcpy(str, "Motion Plus (No Passthrough)");
    break;
  case MOTION_PLUS_NUNCHUK:
    strcpy(str, "Motion Plus (Nunchuk Passthrough Mode)");
    break;
  case MOTION_PLUS_CLASSIC:
    strcpy(str, "Motion Plus (Classic Controller Passthrough Mode)");
    break;
  case NO_DEVICE:
    strcpy(str, "No Device");
    break;
  default:
    strcpy(str, "Unknown Device");
    break;
  }
}