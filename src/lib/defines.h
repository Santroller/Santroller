#pragma once

#include <stdint.h>

#define NONE 0
#define INVALID_PIN 0xFF

#define XBOX_WHAMMY XBOX_R_X
#define XBOX_TILT XBOX_R_Y

// Tilt detection
typedef enum { NO_TILT,
               MPU_6050,
               DIGITAL,
               ANALOGUE } TiltType_t;

// Input types
typedef enum { WII = 1,
               DIRECT,
               PS2 } InputType_t;
// https://docs.microsoft.com/en-us/windows/win32/xinput/xinput-and-controller-subtypes
// http://forum.gimx.fr/viewtopic.php?f=11&t=2897&start=10
typedef enum {
    XINPUT_GAMEPAD = 1,
    XINPUT_WHEEL,
    XINPUT_ARCADE_STICK,
    XINPUT_FLIGHT_STICK,
    XINPUT_DANCE_PAD,
    XINPUT_GUITAR,
    XINPUT_GUITAR_ALTERNATE,
    XINPUT_LIVE_GUITAR,
    XINPUT_DRUMS,
    XINPUT_ARCADE_PAD = 0x13,
    XINPUT_TURNTABLE = 0x17,
} SubType_t;

typedef enum {
    GAMEPAD,
    WHEEL,
    ARCADE_STICK,
    FLIGHT_STICK,
    DANCE_PAD,
    ARCADE_PAD,
    GUITAR_HERO_GUITAR,
    GUITAR_HERO_DRUMS,
    ROCK_BAND_GUITAR,
    ROCK_BAND_DRUMS,
    GUITAR_HERO_LIVE_GUITAR,
    DJ_HERO_TURNTABLE
} DeviceType_t;

typedef enum {
    XBOX360,
    PS3,
    WII_RB,
    SWITCH,
    KEYBOARD_MOUSE,
    MIDI
} ConsoleType_t;

// Orientations for gyros
typedef enum {
    X,
    Y,
    Z
} GyroOrientation_t;

// Firmware types
enum FirmwareType {
    GUITAR_DEVICE_TYPE = 0xfea124,
    ARDWIINO_DEVICE_TYPE = 0xa2d415
};

// Fret Modes
typedef enum { LEDS_DISABLED,
               LEDS_INLINE,
               APA102 } FretLedMode_t;

typedef enum { DISABLED,
               NOTE,
               CONTROL_COMMAND } MidiType_t;

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
} PsxControllerType_t;

#define XBOX_BTN_COUNT 16
#define XBOX_AXIS_COUNT 6
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
enum ControllerAxisPS3 {
    PS3_DPAD_UP,
    PS3_DPAD_RIGHT,
    PS3_DPAD_LEFT,
    PS3_DPAD_DOWN,
    PS3_L2,
    PS3_R2,
    PS3_L1,
    PS3_R1,
    PS3_TRIANGLE,
    PS3_CIRCLE,
    PS3_CROSS,
    PS3_SQUARE
};
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
enum PS3Buttons {
    PS3_TRIANGLE_BT,
    PS3_CIRCLE_BT,
    PS3_CROSS_BT,
    PS3_SQUARE_BT,
    PS3_L2_BT,
    PS3_R2_BT,
    PS3_L1_BT,
    PS3_R1_BT,
    
    PS3_SELECT_BT,
    PS3_START_BT,
    PS3_LEFT_STICK_BT,
    PS3_RIGHT_STICK_BT,
    PS3_PS_BT,
    
    // The HAT doesn't treat the dpad as buttons, but its easier to do that and then transform the hat later.
    PS3_DPAD_UP_BT=0x10,
    PS3_DPAD_DOWN_BT,
    PS3_DPAD_LEFT_BT,
    PS3_DPAD_RIGHT_BT,
};
typedef enum {
    UNUSED, // No Binding
    DIRECT_TYPE, // Direct mode, not used for mouse
    DIRECT_MOUSE, // Direct mode, used for mouse
    OTHER, // Wii/PS2 mode, not used for mouse
    OTHER_MOUSE // Wii/PS2 mode, used for mouse
} BindingType_t;