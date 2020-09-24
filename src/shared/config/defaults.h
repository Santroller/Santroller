#pragma once
#include "../leds/led_colours.h"
#include "./defines.h"
#define CONFIG_VERSION 4
#define TILT_SENSOR NONE
#define DEVICE_TYPE DIRECT
#define OUTPUT_TYPE XINPUT_GUITAR_HERO_GUITAR
#define POLL_RATE 1
// Thresholds
#define TRIGGER_THRESHOLD 64
#define DRUM_THRESHOLD 64
#define JOY_THRESHOLD 64
#define TILT_SENSITIVITY 3000

#define FRET_MODE LEDS_DISABLED
#define COLOUR(col)                                                            \
  ((uint32_t)col >> 16) & 0xff, ((uint32_t)col >> 8) & 0xff,                   \
      ((uint32_t)col) & 0xff
#define LED_PINS                                                               \
  {                                                                            \
    {XBOX_A + 1, COLOUR(Green)}, {XBOX_B + 1, COLOUR(Red)},                    \
        {XBOX_Y + 1, COLOUR(Yellow)}, {XBOX_X + 1, COLOUR(Blue)},              \
        {XBOX_LB + 1, COLOUR(OrangeRed)}, {                                    \
      0                                                                        \
    }                                                                          \
  }
#define MAP_JOY_TO_DPAD true
#define MAP_START_SELECT_TO_HOME true
// When using a nunchunk, map acceleration to r_stick
#define MAP_ACCEL_TO_R true

#define INVALID_AN_PIN                                                         \
  { INVALID_PIN, 0 }
// For a list of keyboard bindings, visit
// http://fourwalledcubicle.com/files/LUFA/Doc/151115/html/group___group___u_s_b_class_h_i_d_common.html

#define EMPTY INVALID_AN_PIN
#define KEYS                                                                   \
  {                                                                            \
    0x04, 0x16, 0x0d, 0x0e, 0x0f, 0x33, 0x28, 0x0b, 0x50, 0x4f, 0x52, 0x51,    \
        0x51, INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN, \
        EMPTY, EMPTY, EMPTY, EMPTY,                                            \
  }

// Set this value to define the orientation of your mpu6050
#define MPU_6050_ORIENTATION NEGATIVE_X
#define PINS                                                                   \
  {                                                                            \
    INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN,        \
        INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN,       \
        INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN,       \
        INVALID_AN_PIN, INVALID_AN_PIN, INVALID_AN_PIN, INVALID_AN_PIN,        \
        INVALID_AN_PIN, INVALID_AN_PIN                                         \
  }
#define FIRMWARE ARDWIINO_DEVICE_TYPE

#define DEFAULT_CONFIG                                                         \
  {                                                                            \
    {DEVICE_TYPE,                                                              \
     OUTPUT_TYPE,                                                              \
     TILT_SENSOR,                                                              \
     POLL_RATE,                                                                \
     FRET_MODE,                                                                \
     MAP_JOY_TO_DPAD,                                                          \
     MAP_START_SELECT_TO_HOME,                                                 \
     MAP_ACCEL_TO_R,                                                           \
     ARDWIINO_DEVICE_TYPE,                                                     \
     CONFIG_VERSION},                                                          \
        PINS,                                                                  \
        {TRIGGER_THRESHOLD, JOY_THRESHOLD, DRUM_THRESHOLD,                     \
         MPU_6050_ORIENTATION, TILT_SENSITIVITY},                              \
        KEYS, LED_PINS, {{0}, {0}, {0}},                                       \
  }
