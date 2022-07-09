#include "defines.h"
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
#define SUPPORTS_LEDS LED_TYPE == LEDS_APA102 || LED_TYPE == LEDS_WS2812
#define SUPPORTS_MIDI CONSOLE_TYPE == MIDI
#define SUPPORTS_HID CONSOLE_TYPE != MIDI
#define SUPPORTS_PICO defined(ARDUINO_ARCH_RP2040)
#define SUPPORTS_AVR defined(__AVR__)
#define SUPPORTS_TEENSY defined(__arm__) && defined(CORE_TEENSY)
#define SUPPORTS_PINS INPUT_TYPE == INPUT_DIRECT
#define SUPPORTS_WII INPUT_TYPE == INPUT_WII
#define SUPPORTS_PS2 INPUT_TYPE == INPUT_PS2
#define SUPPORTS_PASSTHROUGH INPUT_TYPE == INPUT_USBHOST