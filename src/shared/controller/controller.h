#pragma once
#include <stdint.h>

#define XBOX_BTN_COUNT 16
#define XBOX_AXIS_COUNT 6

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
  volatile Led_t leds[XBOX_BTN_COUNT + XBOX_AXIS_COUNT];
  uint8_t drumVelocity[8];
} Controller_t;

// Combined controller structure. Axis' and triggers are combined into arrays to allow for easy modification by direct
typedef struct {
  uint16_t buttons;
  uint8_t triggers[2];
  int16_t sticks[4];
  volatile uint32_t leds[XBOX_BTN_COUNT + XBOX_AXIS_COUNT];
  uint8_t drumVelocity[8];
} ControllerCombined_t;

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
enum WiiExtType {
  WII_NUNCHUK = 0x0000,
  WII_CLASSIC_CONTROLLER = 0x0001,
  WII_CLASSIC_CONTROLLER_PRO = 0x0101,
  WII_THQ_UDRAW_TABLET = 0xFF12,
  WII_UBISOFT_DRAWSOME_TABLET = 0xFF13,
  WII_GUITAR_HERO_GUITAR_CONTROLLER = 0x0003,
  WII_GUITAR_HERO_DRUM_CONTROLLER = 0x0103,
  WII_DJ_HERO_TURNTABLE = 0x0303,
  WII_TAIKO_NO_TATSUJIN_CONTROLLER = 0x0011,
  WII_MOTION_PLUS = 0x0005,
  WII_NO_EXTENSION = 0x180b,
  WII_NOT_INITIALISED = 0xFFFF
};
enum PsxControllerType {
  PSX_UNKNOWN_CONTROLLER = 0,
  PSX_DUALSHOCK_1_CONTROLLER,
  PSX_DUALSHOCK_2_CONTROLLER,
  PSX_WIRELESS_SONY_DUALSHOCK_CONTROLLER,
  PSX_GUITAR_HERO_CONTROLLER,
  PSX_NEGCON,
  PSX_ANALOG,
  PSX_MOUSE,
  PSX_NO_DEVICE
};