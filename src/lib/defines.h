#pragma once

#include <stdint.h>

#define REAL_GUITAR_SUBTYPE 7
#define REAL_DRUM_SUBTYPE 8
#define NONE 0
#define INVALID_PIN 0xFF
#define REAL_GUITAR_SUBTYPE 7
#define REAL_DRUM_SUBTYPE 8

#define XBOX_WHAMMY XBOX_R_X
#define XBOX_TILT XBOX_R_Y

// Tilt detection
enum TiltType { NO_TILT, MPU_6050, DIGITAL, ANALOGUE };

// Input types
enum InputType { WII = 1, DIRECT, PS2 };

enum SubType {
  XINPUT_GAMEPAD = 1,
  XINPUT_WHEEL,
  XINPUT_ARCADE_STICK,
  XINPUT_FLIGHT_STICK,
  XINPUT_DANCE_PAD,
  XINPUT_LIVE_GUITAR = 9,
  XINPUT_ROCK_BAND_DRUMS = 12,
  XINPUT_GUITAR_HERO_DRUMS,
  XINPUT_ROCK_BAND_GUITAR,
  XINPUT_GUITAR_HERO_GUITAR,
  XINPUT_ARCADE_PAD = 19,
  KEYBOARD_GAMEPAD,
  KEYBOARD_GUITAR_HERO_GUITAR,
  KEYBOARD_ROCK_BAND_GUITAR,
  KEYBOARD_LIVE_GUITAR,
  KEYBOARD_GUITAR_HERO_DRUMS,
  KEYBOARD_ROCK_BAND_DRUMS,
  SWITCH_GAMEPAD,
  PS3_GUITAR_HERO_GUITAR,
  PS3_GUITAR_HERO_DRUMS,
  PS3_ROCK_BAND_GUITAR,
  PS3_ROCK_BAND_DRUMS,
  PS3_GAMEPAD,
  WII_ROCK_BAND_GUITAR,
  WII_ROCK_BAND_DRUMS,
  MOUSE,
  MIDI_GAMEPAD,
  MIDI_GUITAR_HERO_GUITAR,
  MIDI_ROCK_BAND_GUITAR,
  MIDI_LIVE_GUITAR,
  MIDI_GUITAR_HERO_DRUMS,
  MIDI_ROCK_BAND_DRUMS
};

// Orientations for gyros
enum GyroOrientation {
  X, Y, Z
};

enum GyroOrientationOld {
  POSITIVE_Z,
  NEGATIVE_Z,
  POSITIVE_Y,
  NEGATIVE_Y,
  POSITIVE_X,
  NEGATIVE_X
};

// Firmware types
enum FirmwareType {
  GUITAR_DEVICE_TYPE = 0xfea124,
  ARDWIINO_DEVICE_TYPE = 0xa2d415
};

// Fret Modes
enum FretLedMode { LEDS_DISABLED, LEDS_INLINE, APA102 };

enum MidiType { DISABLED, NOTE, CONTROL_COMMAND };

enum PinTypeFlags {
  DIGITAL_PIN,
  ANALOGUE_PIN,
  INVERTED,
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
  PSX_JOGCON,
  PSX_GUNCON,
  PSX_ANALOG,
  PSX_MOUSE,
  PSX_NO_DEVICE
};


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