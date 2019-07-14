#include "input_wii_ext.h"
#include "../config/defines.h"
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
uint16_t id;
uint16_t read_ext_id(void) {
  uint8_t data[6];
  twi_readFromPointerSlow(I2C_ADDR, 0xFA, 6, data);
  // 0100 a420 0101 -> #1#######101
  return ((data[0] >> 4) & 0xf) << 12 | (data[4] & 0xf) << 8 | data[5];
}
void write_byte(uint8_t addr, uint8_t data) {
  twi_writeToPointer(I2C_ADDR, addr, 1, &data);
}
void init_controller(void) {
  _delay_us(10);
  write_byte(0xF0, 0x55);
  _delay_us(10);
  write_byte(0xFB, 0x00);
  _delay_us(10);
  id = read_ext_id();
  if (id == GUITAR || id == CLASSIC || id == CLASSIC_PRO || id == GUITAR_2) {
    // Enable high-res mode
    write_byte(0xFE, 0x03);
    _delay_us(10);
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
  twi_readFromPointerSlow(I2C_ADDR, 0x00, sizeof(data), data);
  if (!verifyData(data, sizeof(data))) {
    init_controller();
    return;
  }
  controller->l_x = (data[0] - 0x80) * 256;
  controller->l_y = (data[2] - 0x80) * 256;

  if (id == GUITAR || id == GUITAR_2) {
    int32_t whammy = (data[5] - 0x80) * 256L;
    controller->r_x = constrain(whammy, 0, 32767);
  } else {
    controller->r_x = (data[1] - 0x80) * 256;
    controller->r_y = (data[3] - 0x80) * 256;
    controller->lt = data[4];
    controller->rt = data[5];
  }
  uint16_t buttons = data[6] | (data[7] << 8);
  for (uint8_t i = 2; i < sizeof(classic_bindings); i++) {
    uint8_t idx = classic_bindings[i];
    if (idx == INVALID_PIN) continue;
    bit_write(!bit_check(buttons, i), controller->buttons, idx);
  }
  // for (int i = 0; i < sizeof(data); i++) {
  //   loop_until_bit_is_set(UCSR0A, UDRE0);
  //   UDR0 = data[i];
  // }
}