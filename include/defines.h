#pragma once

#include <stdint.h>

#define NONE 0
#define INVALID_PIN 0xFF

#define ADXL345 1
#define LIS3DH 2
#define MPU6050 3

#define XINPUT_FLAGS_NO_NAV 0xFFFF
#define XINPUT_FLAGS_NONE 0x0000
#define XINPUT_FLAGS_FORCE_FEEDBACK 0x000D

// https://docs.microsoft.com/en-us/windows/win32/xinput/xinput-and-controller-subtypes
// http://forum.gimx.fr/viewtopic.php?f=11&t=2897&start=10
#define XINPUT_GAMEPAD 1
#define XINPUT_WHEEL 2
#define XINPUT_ARCADE_STICK 3
#define XINPUT_FLIGHT_STICK 4
#define XINPUT_DANCE_PAD 5
#define XINPUT_GUITAR 6
#define XINPUT_GUITAR_ALTERNATE 7
#define XINPUT_DRUMS 8
#define XINPUT_STAGE_KIT 9
#define XINPUT_GUITAR_BASS 11
#define XINPUT_PRO_KEYS 15
#define XINPUT_ARCADE_PAD 19
#define XINPUT_TURNTABLE 23
// Not real, but its easier for us to match this way
#define XINPUT_GUITAR_HERO_LIVE 24
#define XINPUT_PRO_GUITAR 25
// Also not real, but nice for matching
#define XINPUT_GUITAR_WT 26
#define XINPUT_DISNEY_INFINITY_AND_LEGO_DIMENSIONS 33
#define XINPUT_SKYLANDERS 36
#define XINPUT_RB_DRUMS 37
#define XINPUT_GH_DRUMS 38

#define PS4_GAMEPAD 0
#define PS4_GUITAR 1
#define PS4_DRUMS 2
#define PS4_FIGHTSTICK 7  // Useful on PS5?

#define GAMEPAD 1
#define DANCE_PAD 2
#define GUITAR_HERO_GUITAR 3
#define ROCK_BAND_GUITAR 4
#define GUITAR_HERO_DRUMS 5
#define ROCK_BAND_DRUMS 6
#define LIVE_GUITAR 7
#define DJ_HERO_TURNTABLE 8
#define STAGE_KIT 9
#define FORTNITE_GUITAR 10
#define FORTNITE_GUITAR_STRUM 11
#define FORTNITE_DRUMS 12
#define GUITAR_HERO_GUITAR_WT 13
#define ROCK_BAND_PRO_KEYS 16
#define ROCK_BAND_PRO_GUITAR_MUSTANG 17
#define ROCK_BAND_PRO_GUITAR_SQUIRE 18
#define KEYBOARD_MOUSE_TYPE 19
#define MIDI_TYPE 20
#define DISNEY_INFINITY 21
#define SKYLANDERS 22
#define LEGO_DIMENSIONS 23
#define TAIKO 24
#define ARCADE 25

#define UNIVERSAL 0
#define XBOX360 1
#define PS3 2
#define WII_RB 3
#define SWITCH 4
#define PS4 5
#define XBOXONE 6
#define OG_XBOX 8
#define WINDOWS 9
// NOTE: if the above ids ever go past 9, bluetooth will need to encode the consoleType differently.
// The below ones are fine as bluetooth will never encode them.
#define KEYBOARD_MOUSE 10
#define MIDI_ID 11
#define BLUETOOTH_REPORT 12
#define SANTROLLER 13
#define RAPHNET 14
#define FNF 15
#define XBOX360_W 16
#define KEYBOARD 17
#define MOUSE 18
#define XBOX360_BB 19
#define PS5 20
#define STREAM_DECK 21
#define GENERIC 22
#define LTEK 23
#define STEPMANIAX 24
#define LTEK_ID 25
#define IOS_FESTIVAL 26
#define PS2 27
#define WII 28
#define PS2_ON_PS3 29
#define STADIA 30
#define XINPUTCOMPAT 31
#define SWITCH2 32
#define NON_CONTROLLER 0xFE
#define UNKNOWN 0xFF

#define STREAM_DECK_OG 1
#define STREAM_DECK_MINI 2
#define STREAM_DECK_XL 3
#define STREAM_DECK_V2 4
#define STREAM_DECK_MK2 5
#define STREAM_DECK_PLUS 6
#define STREAM_DECK_PEDAL 7
#define STREAM_DECK_XLV2 8
#define STREAM_DECK_MINIV2 9
#define STREAM_DECK_NEO 10

#define PINMODE_PULLUP 0
#define PINMODE_PULLDOWN 1
#define PINMODE_FLOATING 2
#define PINMODE_BUS 3

#define LEDS_DISABLED 0
#define LEDS_APA102 1

#define MIDI_TYPE_NOTE 0
#define MIDI_TYPE_CONTROL_COMMAND 1
#define MIDI_CONTROL_COMMAND_MOD_WHEEL 1
#define MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL 64

#define TAP_BAR_UNKNOWN 0
#define TAP_BAR_WT 1
#define TAP_BAR_GH5 2
#define BTN_A 0x01
#define BTN_B 0x02
#define BTN_C 0x03
#define BTN_X 0x04
#define BTN_Y 0x05
#define BTN_Z 0x06
#define BTN_TL 0x07
#define BTN_TR 0x08
#define BTN_TL2 0x09
#define BTN_TR2 0x0a
#define BTN_SELECT 0x0b
#define BTN_START 0x0c
#define BTN_GUIDE 0x0d
#define BTN_THUMBL 0x0e
#define BTN_THUMBR 0x0f
#define BTN_END 0x0f
#define BTN_USAGE(x) 0x09, x

typedef enum {
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
} WiiExtType_t;

typedef enum {
    Announce,
    WaitingDesc1,
    IdentDesc1,
    WaitingDesc,
    IdentDesc,
    WaitingDescEnd,
    IdentDescEnd,
    Auth,
    Ready
} Xbox_One_State_t;

typedef enum {
    Auth1,
    Auth2,
    Auth3,
    Authenticated
} Xbox_360_State_t;
typedef enum {
    PSX_UNKNOWN_CONTROLLER = 0,
    PSX_DIGITAL,
    PSX_DUALSHOCK_1_CONTROLLER,
    PSX_DUALSHOCK_2_CONTROLLER,
    PSX_GUITAR_HERO_CONTROLLER,
    PSX_NEGCON,
    PSX_JOGCON,
    PSX_GUNCON,
    PSX_FLIGHTSTICK,
    PSX_MOUSE,
    PSX_NO_DEVICE
} PsxControllerType_t;

#define RNT_TYPE_NONE_NEW 100
#define RNT_TYPE_CLASSIC 101
#define RNT_TYPE_NUNCHUK 112
#define RNT_TYPE_CLASSIC_PRO 113
#define RNT_TYPE_WIIMOTE_TAIKO 114
#define RNT_TYPE_PSX_DIGITAL 119
#define RNT_TYPE_PSX_ANALOG 120
#define RNT_TYPE_PSX_NEGCON 121
#define RNT_TYPE_PSX_MOUSE 122
#define RNT_TYPE_WII_GUITAR 127
#define RNT_TYPE_UDRAW_TABLET 128
#define RNT_TYPE_WII_DRUM 130

#define GH_MIDI_NOTE_KICK 36
#define GH_MIDI_NOTE_RED 38
#define GH_MIDI_NOTE_GREEN 45
#define GH_MIDI_NOTE_YELLOW 46
#define GH_MIDI_NOTE_BLUE 48
#define GH_MIDI_NOTE_ORANGE 49
#define GH_MIDI_NOTE_KICK2 100

#define RB_MIDI_NOTE_KICK 36
#define RB_MIDI_NOTE_RED 38
#define RB_MIDI_NOTE_GREEN 41
#define RB_MIDI_NOTE_YELLOW 48
#define RB_MIDI_NOTE_BLUE 45
#define RB_MIDI_NOTE_GREEN_CYMBAL 49
#define RB_MIDI_NOTE_YELLOW_CYMBAL 22
#define RB_MIDI_NOTE_BLUE_CYMBAL 51
#define RB_MIDI_NOTE_KICK2 44