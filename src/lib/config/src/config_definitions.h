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
#if LED_TYPE == LEDS_APA102 || LED_TYPE == LEDS_WS2812
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} Led_t;
#endif
#if CONSOLE_TYPE == MIDI
typedef struct {
    uint8_t note : 7;
    bool isCC : 1;
    uint8_t channel : 4;
} Midi_t;
#endif
typedef struct {
    int8_t multiplier;
    int8_t offset;
    int8_t deadzone;
    bool mapToDigital;
    uint8_t trigger;
    uint8_t pin;
} AnalogData_t;
typedef struct {
    uint8_t pin;
    bool pulldown : 1;
    bool pullup : 1;

#if LED_TYPE == LEDS_APA102 || LED_TYPE == LEDS_WS2812
    uint8_t led_index;
    Led_t led_on;
    Led_t led_off;
#endif
#if CONSOLE_TYPE == MIDI
    Midi_t midi;
#endif
    uint8_t milliDeBounce;
    uint8_t analogID;
} Binding_t;
