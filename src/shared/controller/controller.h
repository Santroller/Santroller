#pragma once
#include <stdint.h>

#define XBOX_BTN_COUNT 16
#define XBOX_AXIS_COUNT 6
#define BUFFER_SIZE_QUEUE 255
extern uint16_t wiiExtensionID;
extern uint8_t ps2CtrlType;
typedef struct {
  uint8_t pin;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} Led_t;
// Standard controller structure. axis' and triggers are seperated for easy modification
typedef struct {
  uint16_t buttons;
  uint8_t lt;
  uint8_t rt;
  int16_t l_x;
  int16_t l_y;
  int16_t r_x;
  int16_t r_y;
} Controller_t;
typedef struct {
  uint16_t buttons;
  uint8_t lt;
  uint8_t rt;
  int16_t l_x;
  int16_t l_y;
  uint16_t whammy;
  int16_t r_y;
} Guitar_t;

// Combined controller structure. Axis' and triggers are combined into arrays to allow for easy modification by direct
typedef struct {
  uint16_t buttons;
  uint8_t triggers[2];
  int16_t sticks[4];
} ControllerCombined_t;
enum ControllerAxis {
  XBOX_LT,
  XBOX_RT,
  XBOX_L_X,
  XBOX_L_Y,
  XBOX_R_X,
  XBOX_R_Y
};

#define XBOX_WHAMMY XBOX_R_X
#define XBOX_TILT XBOX_R_Y
// Buttons used by all controllers
enum ControllerButtons {
  XBOX_DPAD_UP,
  XBOX_DPAD_DOWN,
  XBOX_DPAD_LEFT,
  XBOX_DPAD_RIGHT,
  XBOX_START,
  XBOX_BACK,
  XBOX_LEFT_STICK,
  XBOX_RIGHT_STICK,

  XBOX_LB,
  XBOX_RB,
  XBOX_HOME,
  XBOX_UNUSED,
  XBOX_A,
  XBOX_B,
  XBOX_X,
  XBOX_Y,
};

// Buttons used for mapping to a switch or PS3 controller
enum SwitchButtons {
  SWITCH_Y,
  SWITCH_B,
  SWITCH_A,
  SWITCH_X,
  SWITCH_L,
  SWITCH_R,
  SWITCH_ZL,
  SWITCH_ZR,
  SWITCH_SELECT,
  SWITCH_START,
  SWITCH_LEFT_STICK,
  SWITCH_RIGHT_STICK,
  SWITCH_HOME,
  SWITCH_CAPTURE,
};

enum WiiButtons {
  WII_UNUSED_0,
  WII_RT,
  WII_PLUS,
  WII_HOME,
  WII_MINUS,
  WII_LT,
  WII_DPAD_DOWN,
  WII_DPAD_RIGHT,
  WII_DPAD_UP,
  WII_DPAD_LEFT,
  WII_ZR,
  WII_X,
  WII_A,
  WII_Y,
  WII_B,
  WII_ZL,
};