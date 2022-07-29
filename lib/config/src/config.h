#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>

#include "defines.h"


#define CONSOLE_TYPE PC //This can either be PC;MIDI;KEYBOARD_MOUSE
#define DEVICE_TYPE GUITAR_HERO_GUITAR
#define INPUT_TYPE INPUT_DIRECT
#define POLL_RATE 1

#define LED_TYPE LEDS_DISABLED
#define MAP_LEFT_JOYSTICK_TO_DPAD false
#define MAP_START_SELECT_TO_HOME false
#define MAP_NUNCHUK_ACCELERATION_TO_RIGHT_JOY false

#define TILT_TYPE TILT_DISABLED
#define TILT_ORIENTATION GYRO_X

#define RF_ENABLED false
#define RF_ID 0

#define ADC_COUNT 1
#define ADC_PINS {}

#define SKIP_MASK_PICO 0

#define SKIP_MASK_AVR {0}

#define PIN_INIT

#define TICK

// Serialise whatever configuration structure we use on the GUI side, so that we can pull it back
#define CONFIGURATION ""

#include "config_definitions.h"
// ConsoleType can change due to console detection
extern uint8_t consoleType;
#ifdef __cplusplus
}
#endif
