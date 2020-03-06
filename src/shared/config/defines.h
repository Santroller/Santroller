#pragma once
#define NONE 0
#define INVALID_PIN 0xFF

// Tilt detection
enum tilt { NO_TILT, MPU_6050, DIGITAL, ANALOGUE };

// Input types
enum input { WII = 1, DIRECT };

enum subtype {
  XINPUT_GAMEPAD=1,
  XINPUT_WHEEL,
  XINPUT_ARCADE_STICK,
  XINPUT_FLIGHT_STICK,
  XINPUT_DANCE_PAD,
  XINPUT_GUITAR,
  XINPUT_DRUMS,
  XINPUT_GUITAR_BASS=11,
  XINPUT_ARCADE_PAD=19,
  KEYBOARD,
  SWITCH_GAMEPAD,
  PS3_GAMEPAD,
  PS3_GUITAR_HERO_GUITAR,
  PS3_ROCK_BAND_GUITAR,
  PS3_GUITAR_HERO_DRUMS,
  PS3_ROCK_BAND_DRUMS
};



// Orientations for gyros
enum gyro {
    POSITIVE_Z,
    NEGATIVE_Z,
    POSITIVE_Y,
    NEGATIVE_Y,
    POSITIVE_X,
    NEGATIVE_X
};

// Firmware types
enum firmware_type {
    GUITAR_DEVICE_TYPE=0xfea123,
    ARDWIINO_DEVICE_TYPE=0xa2d414
};

// Fret Modes
enum fret_mode {
    LEDS_DISABLED,
    LEDS_INLINE,
    WS2812,
    APA102
};

