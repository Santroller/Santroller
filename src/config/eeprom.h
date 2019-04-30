#pragma once
#include "defaults.h"
#include <avr/eeprom.h>
#include <stdint.h>
#include <LUFA/Drivers/USB/USB.h>
typedef struct {
  uint8_t green;
  uint8_t red;
  uint8_t yellow;
  uint8_t blue;
  uint8_t orange;
  uint8_t start;
  uint8_t select;
  uint8_t whammy;
  uint8_t strum_up;
  uint8_t strum_down;
  uint8_t dpad_left;
  uint8_t dpad_right;
  uint8_t joy_x;
  uint8_t joy_y;
  uint8_t gravity;
} ATTR_PACKED pins_t;
typedef struct {
  uint8_t green;
  uint8_t red;
  uint8_t yellow;
  uint8_t blue;
  uint8_t orange;
  uint8_t up;
  uint8_t down;
  uint8_t left;
  uint8_t right;
  uint8_t start;
  uint8_t select;
  uint8_t whammy;
} ATTR_PACKED keys_t;
typedef struct {
  uint8_t output_type;
  uint8_t input_type;
  uint8_t tilt_type;
  uint8_t subtype;
  uint8_t pollrate;
  pins_t pins;
  uint8_t direction_mode;
  uint16_t whammy_calibration;
  uint16_t mpu_6050_calibration;
  bool frets_led_mode;
  keys_t keys;
} ATTR_PACKED config_t;

const config_t EEMEM config = {OUTPUT_TYPE,
                               DEVICE_TYPE,
                               TILT_SENSOR,
                               XINPUT_SUBTYPE,
                               POLL_RATE,
                               PINS,
                               DIRECTION_MODE,
                               WHAMMY_INITIAL_VALUE,
                               MPU_6050_START,
                               FRETS_LED,
                               KEYS};