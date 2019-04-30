#pragma once
#include "./defines.h"
#define TILT_SENSOR NONE // this can either be none, MPU_6050 or GRAVITY
#define DEVICE_TYPE DIRECT   // this can be either WII or DIRECT
#define OUTPUT_TYPE XINPUT   // this can be either XINPUT or KEYBOARD or GAMEPAD
#define PINS                                                                   \
  { 4, 5, 7, 6, 8, 16, 9, 18, 14, 15, 10, 21, 19, 20, 11 }
#define DIRECTION_MODE JOY
// Set this to JOY if your controller has a joystick, and set Joy X and Joy Y to
// the X and Y for your joystick Set this to DPAD if your controller has a DPAD,
// and set Left and Right to left and right on your dpad. Reuse pins for up and
// down and strumming, if required.
#define INVERT_WHAMMY false
// If your whammy bar appears inverted, set this to true to invert the output of
// your whammy bar.
#define WHAMMY_INITIAL_VALUE 16863
// Set this value to define where 0 is on your whammy bar.
#define FRETS_LED 0
// If this is set to true, the FRET pins will not use pullups, and will require
// a positive voltage to turn on. This allows for the ability to put LEDs in
// series with your frets, and accept HIGH as an input instead of LOW.
// For a list of keyboard bindings, visit
// http://fourwalledcubicle.com/files/LUFA/Doc/151115/html/group___group___u_s_b_class_h_i_d_common.html
#define KEYS                                                                   \
  {                                                                            \
    HID_KEYBOARD_SC_A, HID_KEYBOARD_SC_S, HID_KEYBOARD_SC_J,                   \
        HID_KEYBOARD_SC_K, HID_KEYBOARD_SC_L,                                  \
        HID_KEYBOARD_SC_SEMICOLON_AND_COLON, HID_KEYBOARD_SC_ENTER,            \
        HID_KEYBOARD_SC_H, HID_KEYBOARD_SC_LEFT_ARROW,                         \
        HID_KEYBOARD_SC_RIGHT_ARROW, HID_KEYBOARD_SC_UP_ARROW,                 \
        HID_KEYBOARD_SC_DOWN_ARROW                                             \
  }
#define XINPUT_SUBTYPE GUITAR_SUBTYPE
#define MPU_6050_START 35000