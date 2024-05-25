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
#ifdef CONFIGURABLE_BLOBS
extern const uint8_t* config;
extern const uint8_t* config_blobs;
#else
extern const uint8_t config[CONFIGURATION_LEN];
#endif
#ifdef __cplusplus
}
#endif
#include "defines.h"
#include "reports/controller_reports.h"
#define DEVICE_TYPE_IS_LIVE_GUITAR (DEVICE_TYPE == LIVE_GUITAR)
#define DEVICE_TYPE_IS_GUITAR (DEVICE_TYPE_IS_GAMEPAD && (DEVICE_TYPE == ROCK_BAND_GUITAR || DEVICE_TYPE == GUITAR_HERO_GUITAR))
#define DEVICE_TYPE_IS_DRUM (DEVICE_TYPE_IS_GAMEPAD && (DEVICE_TYPE == ROCK_BAND_DRUMS || DEVICE_TYPE == GUITAR_HERO_DRUMS))
#define DEVICE_TYPE_IS_INSTRUMENT (DEVICE_TYPE_IS_GAMEPAD && (DEVICE_TYPE_IS_GUITAR || DEVICE_TYPE == LIVE_GUITAR || DEVICE_TYPE_IS_DRUM || DEVICE_TYPE == DJ_HERO_TURNTABLE || DEVICE_TYPE == STAGE_KIT || DEVICE_TYPE_IS_PRO))
#define DEVICE_TYPE_IS_KEYBOARD (EMULATION_TYPE == EMULATION_TYPE_KEYBOARD_MOUSE || DEVICE_TYPE == FORTNITE_GUITAR || DEVICE_TYPE == FORTNITE_GUITAR_STRUM || DEVICE_TYPE == FORTNITE_DRUMS)
#define DEVICE_TYPE_IS_GAMEPAD (!(DEVICE_TYPE_IS_KEYBOARD))
#define DEVICE_TYPE_IS_NORMAL_GAMEPAD (DEVICE_TYPE_IS_GAMEPAD)
#define DEVICE_TYPE_IS_PRO (DEVICE_TYPE == ROCK_BAND_PRO_DRUM || DEVICE_TYPE == ROCK_BAND_PRO_GUITAR_MUSTANG || DEVICE_TYPE == ROCK_BAND_PRO_GUITAR_SQUIRE || DEVICE_TYPE == ROCK_BAND_PRO_KEYS)
#if DEVICE_TYPE == GUITAR_HERO_GUITAR || DEVICE_TYPE == DJ_HERO_TURNTABLE || DEVICE_TYPE_IS_PRO
#define PS3_ID 0x06
#elif DEVICE_TYPE == ROCK_BAND_GUITAR || DEVICE_TYPE == ROCK_BAND_DRUMS 
#define PS3_ID 0x00
#else
#define PS3_ID 0x07
#endif
#if DEVICE_TYPE == DANCE_PAD
#define SUB_TYPE XINPUT_DANCE_PAD
#define OG_XBOX_REPORT OGXboxGamepad_Data_t
#define XINPUT_REPORT XInputGamepad_Data_t
#define XBOX_ONE_REPORT XboxOneGamepad_Data_t
#define PC_REPORT PCGamepad_Data_t
#define PS3_REPORT PS3SimpleGamepad_Data_t
#define PS4_REPORT PS4Gamepad_Data_t
#define HID_BUTTON_COUNT 14
#define HID_BUTTON_USAGES \
    BTN_USAGE(BTN_X), \
    BTN_USAGE(BTN_A), \
    BTN_USAGE(BTN_B), \
    BTN_USAGE(BTN_Y), \
    BTN_USAGE(BTN_TL), \
    BTN_USAGE(BTN_TR), \
    BTN_USAGE(BTN_TL2), \
    BTN_USAGE(BTN_TR2), \
    BTN_USAGE(BTN_SELECT), \
    BTN_USAGE(BTN_START), \
    BTN_USAGE(BTN_THUMBL), \
    BTN_USAGE(BTN_THUMBR), \
    BTN_USAGE(BTN_GUIDE), \
    BTN_USAGE(BTN_C)
// exclude pressure
#define HID_AXIS_COUNT 6
#elif DEVICE_TYPE == STAGE_KIT
#define SUB_TYPE XINPUT_STAGE_KIT
#define OG_XBOX_REPORT OGXboxGamepad_Data_t
#define XINPUT_REPORT XInputGamepad_Data_t
#define XBOX_ONE_REPORT XboxOneGamepad_Data_t
#define PC_REPORT PCStageKit_Data_t
#define PS3_REPORT PS3SimpleGamepad_Data_t
#define PS4_REPORT PS4Gamepad_Data_t
#define HID_BUTTON_COUNT 14
// exclude pressure
#define HID_AXIS_COUNT 6
#define HID_BUTTON_USAGES \
    BTN_USAGE(BTN_X), \
    BTN_USAGE(BTN_A), \
    BTN_USAGE(BTN_B), \
    BTN_USAGE(BTN_Y), \
    BTN_USAGE(BTN_TL), \
    BTN_USAGE(BTN_TR), \
    BTN_USAGE(BTN_TL2), \
    BTN_USAGE(BTN_TR2), \
    BTN_USAGE(BTN_SELECT), \
    BTN_USAGE(BTN_START), \
    BTN_USAGE(BTN_THUMBL), \
    BTN_USAGE(BTN_THUMBR), \
    BTN_USAGE(BTN_GUIDE), \
    BTN_USAGE(BTN_C)
#elif DEVICE_TYPE == GAMEPAD
#define SUB_TYPE XINPUT_GAMEPAD
#define OG_XBOX_REPORT OGXboxGamepad_Data_t
#define XINPUT_REPORT XInputGamepad_Data_t
#define XBOX_ONE_REPORT XboxOneGamepad_Data_t
#define PC_REPORT PCGamepad_Data_t
#define PS3_REPORT PS3SimpleGamepad_Data_t
#define PS4_REPORT PS4Gamepad_Data_t
#define HID_BUTTON_COUNT 14
// exclude pressure
#define HID_AXIS_COUNT 6
#define HID_BUTTON_USAGES \
    BTN_USAGE(BTN_X), \
    BTN_USAGE(BTN_A), \
    BTN_USAGE(BTN_B), \
    BTN_USAGE(BTN_Y), \
    BTN_USAGE(BTN_TL), \
    BTN_USAGE(BTN_TR), \
    BTN_USAGE(BTN_TL2), \
    BTN_USAGE(BTN_TR2), \
    BTN_USAGE(BTN_SELECT), \
    BTN_USAGE(BTN_START), \
    BTN_USAGE(BTN_THUMBL), \
    BTN_USAGE(BTN_THUMBR), \
    BTN_USAGE(BTN_GUIDE), \
    BTN_USAGE(BTN_C)
#elif DEVICE_TYPE == GUITAR_HERO_GUITAR
#define SUB_TYPE XINPUT_GUITAR_ALTERNATE
#define PS3_TYPE PS3_GH_GUITAR_PID
#define PS4_TYPE PS4_GUITAR
#define WII_TYPE WII_RB_GUITAR_PID
#define OG_XBOX_REPORT OGXboxGuitarHeroGuitar_Data_t
#define XINPUT_REPORT XInputGuitarHeroGuitar_Data_t
#define XBOX_ONE_REPORT XboxOneRockBandGuitar_Data_t
#define PC_REPORT PCGuitarHeroGuitar_Data_t
#define PS3_REPORT PS3GuitarHeroGuitar_Data_t
#define PS4_REPORT PS4RockBandGuitar_Data_t
#define HID_BUTTON_COUNT 13
#define HID_AXIS_COUNT 3
#define HID_BUTTON_USAGES \
    BTN_USAGE(BTN_A), \
    BTN_USAGE(BTN_B), \
    BTN_USAGE(BTN_Y), \
    BTN_USAGE(BTN_X), \
    BTN_USAGE(BTN_TL), \
    BTN_USAGE(BTN_TR), \
    BTN_USAGE(BTN_SELECT), \
    BTN_USAGE(BTN_START), \
    BTN_USAGE(BTN_GUIDE), \
    BTN_USAGE(BTN_C), \
    BTN_USAGE(BTN_Z), \
    BTN_USAGE(BTN_TL2), \
    BTN_USAGE(BTN_TR2)
#elif DEVICE_TYPE == ROCK_BAND_GUITAR
#define SUB_TYPE XINPUT_GUITAR
#define PS3_TYPE PS3_RB_GUITAR_PID
#define PS4_TYPE PS4_GUITAR
#define WII_TYPE WII_RB_GUITAR_PID
#define OG_XBOX_REPORT OGXboxRockBandGuitar_Data_t
#define XINPUT_REPORT XInputRockBandGuitar_Data_t
#define XBOX_ONE_REPORT XboxOneRockBandGuitar_Data_t
#define PC_REPORT PCRockBandGuitar_Data_t
#define PS3_REPORT PS3RockBandGuitar_Data_t
#define PS4_REPORT PS4RockBandGuitar_Data_t
#define HID_BUTTON_COUNT 13
#define HID_AXIS_COUNT 3
#define HID_BUTTON_USAGES \
    BTN_USAGE(BTN_A), \
    BTN_USAGE(BTN_B), \
    BTN_USAGE(BTN_Y), \
    BTN_USAGE(BTN_X), \
    BTN_USAGE(BTN_TL), \
    BTN_USAGE(BTN_TR), \
    BTN_USAGE(BTN_TL2), \
    BTN_USAGE(BTN_TR2), \
    BTN_USAGE(BTN_C), \
    BTN_USAGE(BTN_Z), \
    BTN_USAGE(BTN_SELECT), \
    BTN_USAGE(BTN_START), \
    BTN_USAGE(BTN_GUIDE)
#elif DEVICE_TYPE == ROCK_BAND_PRO_GUITAR_MUSTANG
#define SUB_TYPE XINPUT_PRO_GUITAR
#define PS3_TYPE PS3_MUSTANG_PID
#define WII_TYPE WII_MUSTANG_PID
#define OG_XBOX_REPORT OGXboxGamepad_Data_t
#define XINPUT_REPORT XInputRockBandProGuitar_Data_t
#define XBOX_ONE_REPORT XboxOneRockBandGuitar_Data_t
#define PC_REPORT PCRockBandProGuitar_Data_t
#define PS3_REPORT PS3RockBandProGuitar_Data_t
#define PS4_REPORT PS3RockBandProGuitar_Data_t
#define HID_BUTTON_COUNT 13
#define HID_AXIS_COUNT 3
#elif DEVICE_TYPE == ROCK_BAND_PRO_GUITAR_SQUIRE
#define SUB_TYPE XINPUT_PRO_GUITAR
#define PS3_TYPE PS3_SQUIRE_PID
#define WII_TYPE WII_SQUIRE_PID
#define OG_XBOX_REPORT OGXboxGamepad_Data_t
#define XINPUT_REPORT XInputRockBandProGuitar_Data_t
#define XBOX_ONE_REPORT XboxOneRockBandGuitar_Data_t
#define PC_REPORT PCRockBandProGuitar_Data_t
#define PS3_REPORT PS3RockBandProGuitar_Data_t
#define PS4_REPORT PS3RockBandProGuitar_Data_t
#define HID_BUTTON_COUNT 13
#define HID_AXIS_COUNT 3
#elif DEVICE_TYPE == ROCK_BAND_PRO_KEYS
#define SUB_TYPE XINPUT_PRO_KEYS
#define PS3_TYPE PS3_KEYBOARD_PID
#define WII_TYPE WII_KEYBOARD_PID
#define OG_XBOX_REPORT OGXboxRockBandKeyboard_Data_t
#define XINPUT_REPORT XInputRockBandKeyboard_Data_t
#define XBOX_ONE_REPORT XboxOneRockBandKeyboard_Data_t
#define PC_REPORT PCRockBandProKeyboard_Data_t
#define PS3_REPORT PS3RockBandProKeyboard_Data_t
#define PS4_REPORT PS3RockBandProKeyboard_Data_t
#define HID_BUTTON_COUNT 35
#define HID_AXIS_COUNT 2
#define HID_BUTTON_USAGES \
    BTN_USAGE(BTN_X), \
    BTN_USAGE(BTN_A), \
    BTN_USAGE(BTN_B), \
    BTN_USAGE(BTN_Y), \
    BTN_USAGE(BTN_SELECT), \
    BTN_USAGE(BTN_START), \
    BTN_USAGE(BTN_GUIDE), \
    BTN_USAGE(BTN_TL), \
    BTN_USAGE(BTN_TR), \
    BTN_USAGE(BTN_C), \
    BTN_USAGE(BTN_TL2), \
    BTN_USAGE(BTN_TR2), \
    BTN_USAGE(BTN_THUMBL), \
    BTN_USAGE(BTN_THUMBR), \
    BTN_USAGE(BTN_Z), \
    BTN_USAGE(BTN_END+1), \
    BTN_USAGE(BTN_END+2), \
    BTN_USAGE(BTN_END+3), \
    BTN_USAGE(BTN_END+4), \
    BTN_USAGE(BTN_END+5), \
    BTN_USAGE(BTN_END+6), \
    BTN_USAGE(BTN_END+7), \
    BTN_USAGE(BTN_END+8), \
    BTN_USAGE(BTN_END+9), \
    BTN_USAGE(BTN_END+10), \
    BTN_USAGE(BTN_END+11), \
    BTN_USAGE(BTN_END+12), \
    BTN_USAGE(BTN_END+13), \
    BTN_USAGE(BTN_END+14), \
    BTN_USAGE(BTN_END+15), \
    BTN_USAGE(BTN_END+16), \
    BTN_USAGE(BTN_END+17), \
    BTN_USAGE(BTN_END+18), \
    BTN_USAGE(BTN_END+19), \
    BTN_USAGE(BTN_END+20)
#elif DEVICE_TYPE == LIVE_GUITAR
#define SUB_TYPE XINPUT_GUITAR_ALTERNATE
#define PS3_TYPE PS3WIIU_GHLIVE_DONGLE_PID
#define OG_XBOX_REPORT OGXboxGHLGuitar_Data_t
#define XINPUT_REPORT XInputGHLGuitar_Data_t
#define XBOX_ONE_REPORT XboxOneGamepad_Data_t
#define PC_REPORT PCGHLGuitar_Data_t
#define PS3_REPORT PS3GHLGuitar_Data_t
#define PS4_REPORT PS4GHLGuitar_Data_t
#define HID_BUTTON_COUNT 14
#define HID_AXIS_COUNT 2
#define HID_BUTTON_USAGES \
    BTN_USAGE(BTN_A), \
    BTN_USAGE(BTN_B), \
    BTN_USAGE(BTN_Y), \
    BTN_USAGE(BTN_X), \
    BTN_USAGE(BTN_TL), \
    BTN_USAGE(BTN_TR), \
    BTN_USAGE(BTN_SELECT), \
    BTN_USAGE(BTN_START), \
    BTN_USAGE(BTN_THUMBL), \
    BTN_USAGE(BTN_GUIDE), \
    BTN_USAGE(BTN_TL2), \
    BTN_USAGE(BTN_TR2), \
    BTN_USAGE(BTN_C), \
    BTN_USAGE(BTN_Z)
#elif DEVICE_TYPE == GUITAR_HERO_DRUMS
#define SUB_TYPE XINPUT_DRUMS
#define PS3_TYPE PS3_GH_DRUM_PID
#define PS4_TYPE PS4_DRUMS
#define WII_TYPE WII_RB_DRUM_PID
#define OG_XBOX_REPORT OGXboxGuitarHeroDrums_Data_t
#define XINPUT_REPORT XInputGuitarHeroDrums_Data_t
#define XBOX_ONE_REPORT XboxOneRockBandDrums_Data_t
#define PC_REPORT PCGuitarHeroDrums_Data_t
#define PS3_REPORT PS3GuitarHeroDrums_Data_t
#define PS4_REPORT PS4RockBandDrums_Data_t
#define HID_BUTTON_COUNT 13
#define HID_AXIS_COUNT 6
#define HID_BUTTON_USAGES \
    BTN_USAGE(BTN_A), \
    BTN_USAGE(BTN_B), \
    BTN_USAGE(BTN_Y), \
    BTN_USAGE(BTN_X), \
    BTN_USAGE(BTN_TL), \
    BTN_USAGE(BTN_TR), \
    BTN_USAGE(BTN_SELECT), \
    BTN_USAGE(BTN_START), \
    BTN_USAGE(BTN_GUIDE), \
    BTN_USAGE(BTN_TL2), \
    BTN_USAGE(BTN_TR2), \
    BTN_USAGE(BTN_C), \
    BTN_USAGE(BTN_Z)
#elif DEVICE_TYPE == ROCK_BAND_DRUMS
#define SUB_TYPE XINPUT_DRUMS
#define PS3_TYPE PS3_RB_DRUM_PID
#define PS4_TYPE PS4_DRUMS
#define WII_TYPE WII_RB_DRUM_PID
#define OG_XBOX_REPORT OGXboxRockBandDrums_Data_t
#define XINPUT_REPORT XInputRockBandDrums_Data_t
#define XBOX_ONE_REPORT XboxOneRockBandDrums_Data_t
#define PC_REPORT PCRockBandDrums_Data_t
#define PS3_REPORT PS3RockBandDrums_Data_t
#define PS4_REPORT PS4RockBandDrums_Data_t
#define HID_BUTTON_COUNT 15
#define HID_AXIS_COUNT 7
#define HID_BUTTON_USAGES \
    BTN_USAGE(BTN_A), \
    BTN_USAGE(BTN_B), \
    BTN_USAGE(BTN_X), \
    BTN_USAGE(BTN_Y), \
    BTN_USAGE(BTN_THUMBR), \
    BTN_USAGE(BTN_TR), \
    BTN_USAGE(BTN_TL), \
    BTN_USAGE(BTN_THUMBL), \
    BTN_USAGE(BTN_SELECT), \
    BTN_USAGE(BTN_START), \
    BTN_USAGE(BTN_GUIDE), \
    BTN_USAGE(BTN_TL2), \
    BTN_USAGE(BTN_TR2), \
    BTN_USAGE(BTN_C), \
    BTN_USAGE(BTN_Z)
#elif DEVICE_TYPE == DJ_HERO_TURNTABLE
#define SUB_TYPE XINPUT_TURNTABLE
#define PS3_TYPE PS3_DJ_TURNTABLE_PID
#define OG_XBOX_REPORT OGXboxTurntable_Data_t
#define XINPUT_REPORT XInputTurntable_Data_t
#define XBOX_ONE_REPORT XboxOneTurntable_Data_t
#define PC_REPORT PCTurntable_Data_t
#define PS3_REPORT PS3Turntable_Data_t
#define PS4_REPORT PS3Turntable_Data_t
#define HID_BUTTON_COUNT 13
#define HID_AXIS_COUNT 4
#define HID_BUTTON_USAGES \
    BTN_USAGE(BTN_A), \
    BTN_USAGE(BTN_B), \
    BTN_USAGE(BTN_X), \
    BTN_USAGE(BTN_Y), \
    BTN_USAGE(BTN_SELECT), \
    BTN_USAGE(BTN_START), \
    BTN_USAGE(BTN_GUIDE), \
    BTN_USAGE(BTN_C), \
    BTN_USAGE(BTN_Z), \
    BTN_USAGE(BTN_TL), \
    BTN_USAGE(BTN_TR), \
    BTN_USAGE(BTN_TL2), \
    BTN_USAGE(BTN_TR2)
#endif
#ifndef PS4_TYPE
#define PS4_TYPE PS4_GAMEPAD
#endif
#if DEVICE_TYPE_IS_NORMAL_GAMEPAD
#ifndef HID_AXIS_COUNT
#error missing HID_AXIS_COUNT and HID_BUTTON_COUNT
#endif
#define HID_BUTTON_PADDING ((HID_BUTTON_COUNT % 8) ? (8 - (HID_BUTTON_COUNT % 8)) : 0)
#endif
// Xbox One Controller config
#define TEMP 0xFF
#if DEVICE_TYPE_IS_GUITAR
#define XBOX_ONE_VID XBOX_ONE_RB_VID
#define XBOX_ONE_PID XBOX_ONE_RB_GUITAR_PID
#define XBOX_ONE_DESCRIPTOR_SIZE 256
#elif DEVICE_TYPE_IS_DRUMS
#define XBOX_ONE_VID XBOX_ONE_RB_VID
#define XBOX_ONE_PID XBOX_ONE_RB_DRUM_PID
#define XBOX_ONE_DESCRIPTOR_SIZE 224
#elif DEVICE_TYPE == LIVE_GUITAR
#define XBOX_ONE_VID XBOX_ONE_GHLIVE_DONGLE_VID
#define XBOX_ONE_PID XBOX_ONE_GHLIVE_DONGLE_PID
#define XBOX_ONE_DESCRIPTOR_SIZE 329
#else
#define XBOX_ONE_VID XBOX_ONE_CONTROLLER_VID
#define XBOX_ONE_PID XBOX_ONE_CONTROLLER_PID
#define XBOX_ONE_DESCRIPTOR_SIZE 244
#endif
typedef union {
    #if DEVICE_TYPE_IS_NORMAL_GAMEPAD
    PS3_REPORT ps3;
    XINPUT_REPORT xinput;
    XBOX_ONE_REPORT xone;
    #endif
    USB_NKRO_Data_t keyboard;
    USB_MIDI_Data_t midi;
    USB_Mouse_Data_t mouse;
} combined_report_t;
extern const uint8_t announce[32];
extern const uint8_t xb1_descriptor[XBOX_ONE_DESCRIPTOR_SIZE];
extern const uint8_t xb1_descriptor_end[6];
#define SUPPORTS_MIDI EMULATION_TYPE == EMULATION_TYPE_MIDI
#define SUPPORTS_HID EMULATION_TYPE != EMULATION_TYPE_MIDI
#define SUPPORTS_PS4 (DEVICE_TYPE == GAMEPAD || DEVICE_TYPE == LIVE_GUITAR || PS4_INSTRUMENT)
#define SUPPORTS_PICO defined(ARDUINO_ARCH_RP2040)
#define SUPPORTS_AVR defined(__AVR__)
#define SUPPORTS_TEENSY (defined(__arm__) && defined(CORE_TEENSY))
#define BLUETOOTH (defined(BLUETOOTH_TX) || defined(BLUETOOTH_RX))
enum hid_reports_t {
    REPORT_ID_NONE,
    REPORT_ID_NKRO,
#ifdef TICK_CONSUMER
    REPORT_ID_CONSUMER,
#endif
#ifdef TICK_MOUSE
    REPORT_ID_MOUSE,
#endif
    REPORT_ID_END
};

typedef struct {
#if DEVICE_TYPE_IS_NORMAL_GAMEPAD
    union {
        PC_REPORT universal;
        PS4_REPORT ps4;
        PS3_REPORT ps3;
        XINPUT_REPORT xinput;
        XBOX_ONE_REPORT xone;
        USB_Host_Data_t host;
        uint8_t data[64];
    } lastControllerReport;
#endif
#ifdef TICK_SIXKRO
    USB_6KRO_Data_t last6KROReport;
#endif
#ifdef TICK_NKRO
    USB_NKRO_Data_t lastNKROReport;
#endif
#ifdef TICK_MOUSE
    USB_Mouse_Data_t lastMouseReport;
#endif
#ifdef TICK_CONSUMER
    USB_ConsumerControl_Data_t lastConsumerReport;
#endif
} USB_LastReport_Data_t;

#define USB_VERSION_BCD(Major, Minor, Revision) \
    (((Major & 0xFF) << 8) |                    \
     ((Minor & 0x0F) << 4) |                    \
     (Revision & 0x0F))

#ifdef CONFIGURABLE_BLOBS
inline int16_t read_int16(uint16_t idx) {
    return *((int16_t*)&config_blobs[idx]);
}
inline uint16_t read_uint16(uint16_t idx) {
    return *((uint16_t*)&config_blobs[idx]);
}
#endif

extern bool disable_multiplexer;