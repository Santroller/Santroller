#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>

#include "defines.h"


#define CONSOLE_TYPE XBOX360 //This can either be PC;MIDI;KEYBOARD_MOUSE
#define DEVICE_TYPE GUITAR_HERO_GUITAR
#define POLL_RATE 1

#define LED_TYPE LEDS_DISABLED

#define TILT_ENABLED false

#define RF_ENABLED false
#define RF_ID 0

#define DIGITAL_COUNT 1
#define ADC_COUNT 1
#define ADC_PINS {}

#define SKIP_MASK_PICO 0

#define SKIP_MASK_AVR {0}

#define PIN_INIT {}

#define TICK {}

#define ARDWIINO_BOARD ""

// Serialise whatever configuration structure we use on the GUI side, so that we can pull it back
#define CONFIGURATION {0}
#define CONFIGURATION_LEN 1

// ConsoleType can change due to console detection
extern const uint8_t consoleType;
extern const uint8_t config[CONFIGURATION_LEN];
#ifdef __cplusplus
}
#endif
#include "defines.h"
#include "controller_reports.h"
#if DEVICE_TYPE == GUITAR_HERO_GUITAR || DEVICE_TYPE == ROCK_BAND_GUITAR || DEVICE_TYPE == GUITAR_HERO_LIVE_GUITAR
#define DEVICE_TYPE_IS_GUITAR true
#define DEVICE_TYPE_IS_DRUM false
#elif DEVICE_TYPE == GUITAR_HERO_DRUMS || DEVICE_TYPE == ROCK_BAND_DRUMS
#define DEVICE_TYPE_IS_GUITAR false
#define DEVICE_TYPE_IS_DRUM true
#else
#define DEVICE_TYPE_IS_GUITAR false
#define DEVICE_TYPE_IS_DRUM false
#endif

#if DEVICE_TYPE == GAMEPAD
#define SUB_TYPE XINPUT_GAMEPAD
#elif DEVICE_TYPE == WHEEL
#define SUB_TYPE XINPUT_WHEEL
#elif DEVICE_TYPE == ARCADE_STICK
#define SUB_TYPE XINPUT_ARCADE_STICK
#elif DEVICE_TYPE == GUITAR_HERO_GUITAR || DEVICE_TYPE == ROCK_BAND_GUITAR
#define SUB_TYPE XINPUT_GUITAR_ALTERNATE
#elif DEVICE_TYPE == GUITAR_HERO_LIVE_GUITAR
#define SUB_TYPE XINPUT_LIVE_GUITAR
#elif DEVICE_TYPE == GUITAR_HERO_DRUMS || DEVICE_TYPE == ROCK_BAND_DRUMS
#define SUB_TYPE XINPUT_DRUMS
#elif DEVICE_TYPE == FLIGHT_STICK
#define SUB_TYPE XINPUT_FLIGHT_STICK
#elif DEVICE_TYPE == DANCE_PAD
#define SUB_TYPE XINPUT_DANCE_PAD
#elif DEVICE_TYPE == ARCADE_PAD
#define SUB_TYPE XINPUT_ARCADE_PAD
#elif DEVICE_TYPE == DJ_HERO_TURNTABLE
#define SUB_TYPE XINPUT_TURNTABLE
#endif
#define SUPPORTS_KEYBOARD CONSOLE_TYPE == KEYBOARD_MOUSE
#define SUPPORTS_LEDS LED_TYPE == LEDS_APA102 || LED_TYPE == LEDS_WS2812
#define SUPPORTS_MIDI CONSOLE_TYPE == MIDI
#define SUPPORTS_HID CONSOLE_TYPE != MIDI
#define SUPPORTS_PICO defined(ARDUINO_ARCH_RP2040)
#define SUPPORTS_AVR defined(__AVR__)
#define SUPPORTS_TEENSY defined(__arm__) && defined(CORE_TEENSY)
#if CONSOLE_TYPE == PC_XINPUT 
#define REPORT_TYPE USB_XInputReport_Data_t
#else
#define REPORT_TYPE USB_PS3Report_Data_t
#endif