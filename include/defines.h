#pragma once

#include <stdint.h>

#define NONE 0
#define INVALID_PIN 0xFF

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
#define XINPUT_ARCADE_PAD 19
#define XINPUT_TURNTABLE 23

#define GAMEPAD 1
#define WHEEL 2
#define ARCADE_STICK 3
#define FLIGHT_STICK 4
#define DANCE_PAD 5
#define ARCADE_PAD 6
#define GUITAR 7
#define LIVE_GUITAR 8
#define DRUMS 9
#define DJ_HERO_TURNTABLE 10
#define STAGE_KIT 11

#define GUITAR_HERO 0
#define ROCK_BAND 1

#define UNIVERSAL 0
#define XBOX360 1
#define PS3 2
#define WII_RB 3
#define SWITCH 4
#define PS4 5
#define XBOXONE 6
#define REAL_PS3 7
#define WINDOWS 9
// NOTE: if the above ids ever go past 9, bluetooth will need to encode the consoleType differently.
// The below ones are fine as bluetooth will never encode them.
#define KEYBOARD_MOUSE 10
#define MIDI 11
#define BLUETOOTH_REPORT 12
#define UNKNOWN 0xFF

#define PINMODE_PULLUP 0
#define PINMODE_PULLDOWN 1
#define PINMODE_FLOATING 2
#define PINMODE_BUS 3

#define LEDS_DISABLED 0 
#define LEDS_APA102 1
#define LEDS_WS2812 2

#define MIDI_TYPE_NOTE 0
#define MIDI_TYPE_CONTROL_COMMAND 1

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
    Waiting1,
    Ident1,
    Waiting2,
    Ident2,
    Ident3,
    Ident4,
    Waiting5,
    Ident5,
    Auth,
    Ready
} Xbox_One_State_t;

typedef enum {
    Auth1,
    Auth2,
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
