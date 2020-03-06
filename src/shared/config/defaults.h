#pragma once
#include "./defines.h"
#define PROTOCOL_VERSION 0
#define TILT_SENSOR NONE
#define DEVICE_TYPE WII
#define OUTPUT_TYPE XINPUT_GUITAR
#define POLL_RATE 1
// Thresholds
#define TRIGGER_THRESHOLD 12767
#define JOY_THRESHOLD 12767
#define TILT_SENSITIVITY 3000

#define FRET_MODE LEDS_DISABLED

#define MAP_JOY_TO_DPAD true
#define MAP_START_SELECT_TO_HOME true
// When using a nunchunk, map acceleration to r_stick
#define MAP_ACCEL_TO_R true
#define CONFIG_VERSION 0
// If this is set to true, the FRET pins will not use pullups, and will require
// a positive voltage to turn on. This allows for the ability to put LEDs in
// series with your frets, and accept HIGH as an input instead of LOW.
// For a list of keyboard bindings, visit
// http://fourwalledcubicle.com/files/LUFA/Doc/151115/html/group___group___u_s_b_class_h_i_d_common.html
#define EMPTY                                                                  \
  { INVALID_PIN, INVALID_PIN }
#define KEYS                                                                   \
  {                                                                            \
    0x04, 0x16, 0x0d, 0x0e, 0x0f, 0x33, 0x28, 0x0b, 0x50, 0x4f, 0x52, 0x51,    \
        0x51, INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN, \
        EMPTY, EMPTY, EMPTY, EMPTY,                                            \
  }

// Set this value to define the orientation of your mpu6050
#define MPU_6050_ORIENTATION NEGATIVE_X
#if defined(__AVR_ATmega328P__)
#  define MPU_6050_INTERRUPT_PIN 2
#else
#  define MPU_6050_INTERRUPT_PIN 10
#endif
#define PINS                                                                   \
  {                                                                            \
    21, 16, INVALID_PIN, INVALID_PIN, 15, 14, INVALID_PIN, INVALID_PIN, 9,     \
        INVALID_PIN, INVALID_PIN, INVALID_PIN, 5, 6, 8, 7,                     \
        {INVALID_PIN, false}, {INVALID_PIN, false}, {19, false}, {20, false},  \
        {18, true}, {                                                          \
      MPU_6050_INTERRUPT_PIN, false                                            \
    }                                                                          \
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
     MAP_ACCEL_TO_R, ARDWIINO_DEVICE_TYPE, CONFIG_VERSION},                                                          \
        PINS,                                                                  \
        {TRIGGER_THRESHOLD, JOY_THRESHOLD, MPU_6050_ORIENTATION,               \
         TILT_SENSITIVITY},                                                    \
        KEYS,                                                                  \
  }
