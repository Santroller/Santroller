#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>

#include "defines.h"
#if __has_include("config_data.h")
#include "config_data.h"
#endif

// ConsoleType can change due to console detection
extern uint8_t consoleType;
extern const uint8_t config[CONFIGURATION_LEN];
#ifdef __cplusplus
}
#endif
#include "defines.h"
#include "reports/controller_reports.h"
#if DEVICE_TYPE == LIVE_GUITAR
#define DEVICE_TYPE_IS_LIVE_GUITAR true
#else
#define DEVICE_TYPE_IS_LIVE_GUITAR false
#endif
#if DEVICE_TYPE == GUITAR || DEVICE_TYPE == LIVE_GUITAR
#define DEVICE_TYPE_IS_GUITAR true
#define DEVICE_TYPE_IS_DRUM false
#elif DEVICE_TYPE == DRUMS
#define DEVICE_TYPE_IS_GUITAR false
#define DEVICE_TYPE_IS_DRUM true
#else
#define DEVICE_TYPE_IS_GUITAR false
#define DEVICE_TYPE_IS_DRUM false
#endif
#define DEVICE_TYPE_IS_INSTRUMENT (DEVICE_TYPE == GUITAR || DEVICE_TYPE == LIVE_GUITAR || DEVICE_TYPE == DRUMS || DEVICE_TYPE == DJ_HERO_TURNTABLE)
#define DEVICE_TYPE_KEYBOARD CONSOLE_TYPE == KEYBOARD_MOUSE
#define DEVICE_TYPE_IS_GAMEPAD CONSOLE_TYPE != KEYBOARD_MOUSE
#if DEVICE_TYPE == GUITAR || DEVICE_TYPE == DRUMS
#if RHYTHM_TYPE == GUITAR_HERO
#define PS3_ID 0x06
#elif RHYTHM_TYPE == ROCK_BAND
#define PS3_ID 0x00
#endif
#else
#define PS3_ID 0x07
#endif
#if DEVICE_TYPE == STAGE_KIT
#define SUB_TYPE XINPUT_STAGE_KIT
#define XINPUT_FLAGS 0x00
#define XINPUT_REPORT XInputGamepad_Data_t
#define XBOX_ONE_REPORT XboxOneGamepad_Data_t
#define PS3_REPORT PCGamepad_Data_t
#define PS4_REPORT PS4Gamepad_Data_t
#elif DEVICE_TYPE == GAMEPAD || DEVICE_TYPE == ARCADE_STICK || DEVICE_TYPE == FLIGHT_STICK || DEVICE_TYPE == DANCE_PAD || DEVICE_TYPE == ARCADE_PAD
#define SUB_TYPE XINPUT_GAMEPAD
#define XINPUT_FLAGS 0x10
#define XINPUT_REPORT XInputGamepad_Data_t
#define XBOX_ONE_REPORT XboxOneGamepad_Data_t
#define PS3_REPORT PCGamepad_Data_t
#define PS4_REPORT PS4Gamepad_Data_t
#elif DEVICE_TYPE == GUITAR && RHYTHM_TYPE == GUITAR_HERO
#define SUB_TYPE XINPUT_GUITAR_ALTERNATE
#define PS3_TYPE PS3_GH_GUITAR_PID
#define WII_TYPE WII_RB_GUITAR_PID
#define XINPUT_FLAGS 0x10
#define XINPUT_REPORT XInputGuitarHeroGuitar_Data_t
#define XBOX_ONE_REPORT XboxOneRockBandGuitar_Data_t
#define PS3_REPORT PS3GuitarHeroGuitar_Data_t
#define PS4_REPORT PS3GuitarHeroGuitar_Data_t
#elif DEVICE_TYPE == GUITAR && RHYTHM_TYPE == ROCK_BAND
#define SUB_TYPE XINPUT_GUITAR
#define PS3_TYPE PS3_RB_GUITAR_PID
#define WII_TYPE WII_RB_GUITAR_PID
#define XINPUT_FLAGS 0x10
#define XINPUT_REPORT XInputRockBandGuitar_Data_t
#define XBOX_ONE_REPORT XboxOneRockBandGuitar_Data_t
#define PS3_REPORT PS3RockBandGuitar_Data_t
#define PS4_REPORT PS3RockBandGuitar_Data_t
#elif DEVICE_TYPE == LIVE_GUITAR
#define SUB_TYPE XINPUT_GUITAR_ALTERNATE
#define PS3_TYPE PS3WIIU_GHLIVE_DONGLE_PID
#define XINPUT_FLAGS 0x00
#define XINPUT_REPORT XInputGHLGuitar_Data_t
#define XBOX_ONE_REPORT XboxOneGamepad_Data_t
#define PS3_REPORT PS3GHLGuitar_Data_t
#define PS4_REPORT PS4GHLGuitar_Data_t
#elif DEVICE_TYPE == DRUMS && RHYTHM_TYPE == GUITAR_HERO
#define SUB_TYPE XINPUT_DRUMS
#define PS3_TYPE PS3_GH_DRUM_PID
#define WII_TYPE WII_RB_DRUM_PID
#define XINPUT_FLAGS 0x10
#define XINPUT_REPORT XInputGuitarHeroDrums_Data_t
#define XBOX_ONE_REPORT XboxOneRockBandDrums_Data_t
#define PS3_REPORT PS3GuitarHeroDrums_Data_t
#define PS4_REPORT PS3GuitarHeroDrums_Data_t
#elif DEVICE_TYPE == DRUMS && RHYTHM_TYPE == ROCK_BAND
#define SUB_TYPE XINPUT_DRUMS
#define PS3_TYPE PS3_RB_DRUM_PID
#define WII_TYPE WII_RB_DRUM_PID
#define XINPUT_FLAGS 0x10
#define XINPUT_REPORT XInputRockBandDrums_Data_t
#define XBOX_ONE_REPORT XboxOneRockBandDrums_Data_t
#define PS3_REPORT PS3RockBandDrums_Data_t
#define PS4_REPORT PS3RockBandDrums_Data_t
#elif DEVICE_TYPE == DJ_HERO_TURNTABLE
#define SUB_TYPE XINPUT_TURNTABLE
#define PS3_TYPE PS3_DJ_TURNTABLE_PID
#define XINPUT_FLAGS 0x00
#define XINPUT_REPORT XInputTurntable_Data_t
#define XBOX_ONE_REPORT XInputTurntable_Data_t
#define PS3_REPORT PS3Turntable_Data_t
#define PS4_REPORT PS3Turntable_Data_t
#endif
// Xbox One Controller config
#define TEMP 0xFF
#if DEVICE_TYPE == GUITAR
#define XBOX_ONE_VID XBOX_ONE_RB_VID
#define XBOX_ONE_PID XBOX_ONE_RB_GUITAR_PID
#define IDENTIFY_4_SIZE 64
#elif DEVICE_TYPE == DRUMS
#define XBOX_ONE_VID XBOX_ONE_RB_VID
#define XBOX_ONE_PID XBOX_ONE_RB_DRUM_PID
#define IDENTIFY_4_SIZE 64
#elif DEVICE_TYPE == LIVE_GUITAR
#define XBOX_ONE_VID XBOX_ONE_GHLIVE_DONGLE_VID
#define XBOX_ONE_PID XBOX_ONE_GHLIVE_DONGLE_PID
#define IDENTIFY_4_SIZE 64
#else
#define XBOX_ONE_VID XBOX_ONE_CONTROLLER_VID
#define XBOX_ONE_PID XBOX_ONE_CONTROLLER_PID
#define IDENTIFY_4_SIZE 46
#endif
#if defined(RF_RX) || defined(RF_TX)
#define RF
#endif
typedef union {
    PS3_REPORT ps3;
    XINPUT_REPORT xinput;
    XBOX_ONE_REPORT xone;
    USB_NKRO_Data_t keyboard;
    USB_MIDI_Data_t midi;
    USB_Mouse_Data_t mouse;
} combined_report_t;
extern const uint8_t announce[32];
extern const uint8_t identify_1[64];
extern const uint8_t identify_2[64];
extern const uint8_t identify_3[64];
extern const uint8_t identify_4[IDENTIFY_4_SIZE];
extern const uint8_t identify_5[8];
#define SUPPORTS_LEDS LED_TYPE == LEDS_APA102 || LED_TYPE == LEDS_WS2812
#define SUPPORTS_MIDI CONSOLE_TYPE == MIDI
#define SUPPORTS_HID CONSOLE_TYPE != MIDI
#define SUPPORTS_PS4 (DEVICE_TYPE != GUITAR && DEVICE_TYPE != DRUMS && DEVICE_TYPE != DJ_HERO_TURNTABLE)
#define SUPPORTS_PICO defined(ARDUINO_ARCH_RP2040)
#define SUPPORTS_AVR defined(__AVR__)
#define SUPPORTS_TEENSY (defined(__arm__) && defined(CORE_TEENSY))
#define BLUETOOTH (defined(BLUETOOTH_TX) || defined(BLUETOOTH_RX))
enum hid_reports_t {
    REPORT_ID_NONE,
#if DEVICE_TYPE_IS_GAMEPAD
    REPORT_ID_CONTROLLER,
#endif
#ifdef TICK_NKRO
    REPORT_ID_NKRO,
#endif
#ifdef TICK_CONSUMER
    REPORT_ID_CONSUMER,
#endif
#ifdef TICK_MOUSE
    REPORT_ID_MOUSE,
#endif
    REPORT_ID_END,
    REPORT_ID_CONSOLE_MODE
};

typedef struct {
    uint8_t report_id;
    PS3_REPORT report;
} __attribute__((packed)) PS3Universal_Data_t;
typedef struct {
#ifdef TICK_NKRO
    USB_NKRO_Data_t lastNKROReport;
#endif
#ifdef TICK_MOUSE
    USB_Mouse_Data_t lastMouseReport;
#endif
#ifdef TICK_CONSUMER
    USB_ConsumerControl_Data_t lastConsumerReport;
#endif
#define USB_VERSION_BCD(Major, Minor, Revision) \
    (((Major & 0xFF) << 8) |                    \
     ((Minor & 0x0F) << 4) |                    \
     (Revision & 0x0F))

#if DEVICE_TYPE_IS_GAMEPAD
    union {
        PS3Universal_Data_t universal;
        PS4_REPORT ps4;
        PS3_REPORT ps3;
        XINPUT_REPORT xinput;
        XBOX_ONE_REPORT xone;
    } lastControllerReport;
#endif
} USB_LastReport_Data_t;
