#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "enums.pb.h"

#define SERIAL_NUMBER_WVALUE 0x0000
#define INPUT_CAPABILITIES_WVALUE 0x0100
#define VIBRATION_CAPABILITIES_WVALUE 0x0000

#define SERIAL_NUMBER_WVALUE 0x0000
#define INPUT_CAPABILITIES_WVALUE 0x0100
#define VIBRATION_CAPABILITIES_WVALUE 0x0000
#define XINPUT_DESC_TYPE_RESERVED 0x21
#define XINPUT_DESC_TYPE_WIRELESS_CAPABILITIES 0x22
#define XINPUT_SECURITY_DESC_TYPE_RESERVED 0x41
#define XBOX_LED_ID 0x01
#define XBOX_RUMBLE_ID 0x00
typedef struct
{
    uint8_t bLength;         // Length of this descriptor.
    uint8_t bDescriptorType; // XINPUT_DESC_TYPE_RESERVED
    uint8_t reserved[2];
    uint8_t subtype;
    uint8_t reserved2;
    uint8_t bEndpointAddressIn;
    uint8_t bMaxDataSizeIn;
    uint8_t reserved3[5];
    uint8_t bEndpointAddressOut;
    uint8_t bMaxDataSizeOut;
    uint8_t reserved4[2];
} __attribute__((packed)) XBOX_ID_DESCRIPTOR;
typedef struct
{
    uint8_t bLength;         // Length of this descriptor.
    uint8_t bDescriptorType; // XINPUT_SECURITY_DESC_TYPE_RESERVED
    uint8_t reserved[4];
} __attribute__((packed)) XBOX_SECURITY_DESCRIPTOR;

enum
{
    XINPUT_GAMEPAD = 1,
    XINPUT_WHEEL = 2,
    XINPUT_ARCADE_STICK = 3,
    XINPUT_FLIGHT_STICK = 4,
    XINPUT_DANCE_PAD = 5,
    XINPUT_GUITAR = 6,
    XINPUT_GUITAR_ALTERNATE = 7,
    XINPUT_DRUMS = 8,
    XINPUT_STAGE_KIT = 9,
    XINPUT_GUITAR_BASS = 11,
    XINPUT_PRO_KEYS = 15,
    XINPUT_ARCADE_PAD = 19,
    XINPUT_TURNTABLE = 23,
    XINPUT_PRO_GUITAR = 25,
    XINPUT_DISNEY_INFINITY_AND_LEGO_DIMENSIONS = 33,
    XINPUT_SKYLANDERS = 36
};

typedef enum
{
    Controller,
    Security,
    Chatpad,
    Audio
} XInputDeviceType;

inline SubType get_subtype_from_xinput(uint8_t subtype)
{
    switch (subtype)
    {
    case XINPUT_ARCADE_PAD:
    case XINPUT_GAMEPAD:
        return Gamepad;
    case XINPUT_WHEEL:
        return Wheel;
    case XINPUT_ARCADE_STICK:
        return FightStick;
    case XINPUT_FLIGHT_STICK:
        return FlightStick;
    case XINPUT_DANCE_PAD:
        return Dancepad;
    case XINPUT_GUITAR:
        return RockBandGuitar;
    case XINPUT_GUITAR_ALTERNATE:
        return GuitarHeroGuitar;
    case XINPUT_DRUMS:
        return RockBandDrums;
    case XINPUT_STAGE_KIT:
        return StageKit;
    case XINPUT_GUITAR_BASS:
        return RockBandGuitar;
    case XINPUT_PRO_KEYS:
        return ProKeys;
    case XINPUT_TURNTABLE:
        return DjHeroTurntable;
    case XINPUT_PRO_GUITAR:
        return ProGuitarSquire;
    case XINPUT_DISNEY_INFINITY_AND_LEGO_DIMENSIONS:
        return DisneyInfinity;
    case XINPUT_SKYLANDERS:
        return Skylanders;
    }
    return Gamepad;
}
inline uint8_t get_xinput_subtype(SubType subtype)
{
    switch (subtype)
    {
    case Gamepad:
    case PopNMusic:
    case ProjectDiva:
    case DJMax:
        return XINPUT_GAMEPAD;
    case Dancepad:
        return XINPUT_DANCE_PAD;
    case GuitarFreaks:
    case LiveGuitar:
    case GuitarHeroGuitar:
    case RockRevolutionGuitar:
        return XINPUT_GUITAR_ALTERNATE;
    case RockBandGuitar:
    case PowerGigGuitar:
        return XINPUT_GUITAR;
    case GuitarHeroDrums:
    case RockBandDrums:
    case PowerGigDrum:
        return XINPUT_DRUMS;
    case DjHeroTurntable:
        return XINPUT_TURNTABLE;
    case ProGuitarMustang:
    case ProGuitarSquire:
        return XINPUT_PRO_GUITAR;
    case ProKeys:
        return XINPUT_PRO_KEYS;
    case Taiko:
        return XINPUT_GAMEPAD;
    case StageKit:
        return XINPUT_STAGE_KIT;
    case KeyboardMouse:
        return XINPUT_GAMEPAD;
    case Wheel:
        return XINPUT_WHEEL;
    case DisneyInfinity:
    case LegoDimensions:
        return XINPUT_DISNEY_INFINITY_AND_LEGO_DIMENSIONS;
    case Skylanders:
        return XINPUT_SKYLANDERS;
    case FightStick:
        return XINPUT_ARCADE_STICK;
    case FlightStick:
        return XINPUT_FLIGHT_STICK;
    }
    return 1;
}

#define XINPUT_FLAGS_NO_NAV 0x0010
#define XINPUT_FLAGS_NONE 0x0000
#define XINPUT_FLAGS_FORCE_FEEDBACK 0x0001

#define LED_OFF 0
#define LED_ALL_BLINKING 1
#define LED_ONE_FLASH 2
#define LED_TWO_FLASH 3
#define LED_THREE_FLASH 4
#define LED_FOUR_FLASH 5
#define LED_ONE 6
#define LED_TWO 7
#define LED_THREE 8
#define LED_FOUR 9
#define LED_ROTATING 10
#define LED_BLINKING_RET 11
#define LED_SLOW_BLINKING_RET 12
#define LED_ALTERNATING_RE 13

#define BB_GREEN 0
#define BB_RED 0
#define BB_BLUE 0
#define BB_YELLOW 0

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t led;
} __attribute__((packed)) XInputLEDReport_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t unused;
    uint8_t leftRumble;
    uint8_t rightRumble;
    uint8_t unused2[3];
} __attribute__((packed)) XInputRumbleReport_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t padding;
    uint8_t left_motor;
    uint8_t right_motor;
    uint8_t padding_2[3];
} __attribute__((packed)) XInputVibrationCapabilities_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint16_t buttons;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint16_t leftThumbX;
    uint16_t leftThumbY;
    uint16_t rightThumbX;
    uint16_t rightThumbY;
    uint8_t reserved[4];
    uint16_t flags;
} __attribute__((packed)) XInputInputCapabilities_t;

typedef struct
{
    uint32_t serial;
} __attribute__((packed)) XInputSerialNumber_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t controllerNumber;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t leftThumbClick : 1;
    uint8_t rightThumbClick : 1;

    uint8_t leftShoulder : 1;
    uint8_t rightShoulder : 1;
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1;
    uint8_t reserved;
} __attribute__((packed)) XInputBigButton_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t leftThumbClick : 1;
    uint8_t rightThumbClick : 1;

    uint8_t leftShoulder : 1;
    uint8_t rightShoulder : 1;
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1;

    uint8_t leftTrigger;
    uint8_t rightTrigger;
    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;
    uint8_t reserved_1[6];
} __attribute__((packed)) XInputGamepad_Data_t;

typedef struct
{
    uint8_t rid;
    uint16_t leftStickX;
    uint16_t leftStickY;
    uint16_t rightStickX;
    uint16_t rightStickY;
    uint16_t leftTrigger;
    uint16_t rightTrigger;

    uint8_t dpad : 4;
    uint8_t : 4;

    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1;
    uint8_t leftShoulder : 1;
    uint8_t rightShoulder : 1;
    uint8_t back : 1;
    uint8_t start : 1;

    uint8_t leftThumbClick : 1;
    uint8_t rightThumbClick : 1;
    uint8_t : 6;
} __attribute__((packed)) XInputCompatGamepad_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t guide : 1;
    uint8_t : 7;
} __attribute__((packed)) XInputCompatGuide_Data_t;
typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t kick2 : 1;   // pedal2
    uint8_t padFlag : 1; // right thumb click

    uint8_t kick1 : 1;      // pedal1
    uint8_t cymbalFlag : 1; // right shoulder click
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1; // green
    uint8_t b : 1; // red
    uint8_t x : 1; // blue
    uint8_t y : 1; // yellow

    uint8_t unused[2];
    int16_t redVelocity;
    int16_t yellowVelocity;
    int16_t blueVelocity;
    int16_t greenVelocity;
    uint8_t reserved_1[6];
} __attribute__((packed)) XInputRockBandDrums_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t leftThumbClick : 1; // isGuitarHero
    uint8_t : 1;

    uint8_t leftShoulder : 1;  // kick
    uint8_t rightShoulder : 1; // orange
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1; // green
    uint8_t b : 1; // red
    uint8_t x : 1; // blue
    uint8_t y : 1; // yellow

    uint8_t unused1[2];
    int16_t leftStickX;
    union
    {
        struct
        {
            uint8_t greenVelocity;
            uint8_t redVelocity; // redVelocity stores the velocity for the cymbal if both cymbal and pad of the same colour get hit simultaneously
        };
        int16_t leftStickY;
    };
    uint8_t yellowVelocity;
    uint8_t blueVelocity;
    uint8_t orangeVelocity;
    uint8_t kickVelocity;
    uint8_t midiPacket[6]; // 0x99 note, velocity, xxxx
} __attribute__((packed)) XInputGuitarHeroDrums_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;   // dpadStrumUp
    uint8_t dpadDown : 1; // dpadStrumDown
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t leftShoulder : 1;  // orange
    uint8_t rightShoulder : 1; // pedal
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1; // green
    uint8_t b : 1; // red
    uint8_t x : 1; // blue
    uint8_t y : 1; // yellow

    uint8_t accelZ;
    uint8_t accelX;
    union
    {
        struct
        {
            int16_t tapGreen : 1;
            int16_t tapRed : 1;
            int16_t tapYellow : 1;
            int16_t tapBlue : 1;
            int16_t tapOrange : 1;
            int16_t : 11;
        };
        int16_t slider;
    };
    int16_t leftStickY;
    int16_t whammy;
    int16_t tilt;
    uint8_t reserved_1[6];
} __attribute__((packed)) XInputGuitarHeroGuitar_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;   // dpadStrumUp
    uint8_t dpadDown : 1; // dpadStrumDown
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t solo : 1; // leftThumbClick
    uint8_t : 1;

    uint8_t leftShoulder : 1; // orange
    uint8_t : 1;
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1; // green
    uint8_t b : 1; // red
    uint8_t x : 1; // blue
    uint8_t y : 1; // yellow

    uint8_t pickup;
    uint8_t unused1;
    int16_t calibrationSensor;
    int16_t leftStickY;
    int16_t whammy;
    int16_t tilt;
    uint8_t reserved_1[6];
} __attribute__((packed)) XInputRockBandGuitar_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;   // dpadStrumUp
    uint8_t dpadDown : 1; // dpadStrumDown
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t solo : 1; // leftThumbClick
    uint8_t : 1;

    uint8_t leftShoulder : 1; // orange
    uint8_t : 1;
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1; // green
    uint8_t b : 1; // red
    uint8_t x : 1; // blue
    uint8_t y : 1; // yellow

    uint16_t lowEFret : 5;
    uint16_t aFret : 5;
    uint16_t dFret : 5;
    uint16_t : 1;
    uint16_t gFret : 5;
    uint16_t bFret : 5;
    uint16_t highEFret : 5;
    uint16_t soloFlag : 1;

    union
    {
        struct
        {
            uint16_t lowEFretVelocity : 7;
            uint16_t green : 1;
            uint16_t aFretVelocity : 7;
            uint16_t red : 1;
            uint16_t dFretVelocity : 7;
            uint16_t yellow : 1;
            uint16_t gFretVelocity : 7;
            uint16_t blue : 1;
            uint16_t bFretVelocity : 7;
            uint16_t orange : 1;
            uint16_t highEFretVelocity : 7;
            uint16_t : 1;
        };
        struct
        {
            int16_t leftStickX;
            int16_t leftStickY;
        };
    };

    uint8_t autoCal_Microphone; // When the sensor isn't activated, this
    uint8_t autoCal_Light;      // and this just duplicate the tilt axis
    uint8_t tilt;

    uint8_t : 7;
    uint8_t pedal : 1;

    uint8_t unused2;

    uint8_t pedalConnection : 1;
    uint8_t : 7;
} __attribute__((packed)) XInputRockBandProGuitar_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;   // dpadStrumUp
    uint8_t dpadDown : 1; // dpadStrumDown
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t solo : 1; // leftThumbClick
    uint8_t : 1;

    uint8_t leftShoulder : 1; // orange
    uint8_t : 1;
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1; // green
    uint8_t b : 1; // red
    uint8_t x : 1; // blue
    uint8_t y : 1; // yellow

    uint8_t key1;
    uint8_t key2;
    uint8_t key3;

    uint8_t velocities[5];

    uint8_t : 7;
    uint8_t overdrive : 1;
    uint8_t pedalAnalog : 7;
    uint8_t pedalDigital : 1;

    uint8_t touchPad : 7;
    uint8_t : 1;

    uint8_t unused2[4];

    uint8_t pedalConnection : 1; // If this matches PS3 MPA behavior, always 0 with the MIDI Pro Adapter
    uint8_t : 7;
} __attribute__((packed)) XInputRockBandKeyboard_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;          // start, pause
    uint8_t back : 1;           // back, heroPower
    uint8_t leftThumbClick : 1; // leftThumbClick, ghtv
    uint8_t : 1;

    uint8_t leftShoulder : 1;  // white2 leftShoulder
    uint8_t rightShoulder : 1; // white3 rightShoulder
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1; // black1 a
    uint8_t b : 1; // black2 b
    uint8_t x : 1; // white1 x
    uint8_t y : 1; // black3 y

    uint8_t unused1[2];

    int16_t leftStickX;
    int16_t strumBar;
    int16_t tilt;
    int16_t whammy;

    uint8_t reserved_1[6];
} __attribute__((packed)) XInputGHLGuitar_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t : 1;
    uint8_t : 1;
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1; // euphoria

    uint8_t leftGreen : 1;
    uint8_t leftRed : 1;
    uint8_t leftBlue : 1;
    uint8_t : 5;

    uint8_t rightGreen : 1;
    uint8_t rightRed : 1;
    uint8_t rightBlue : 1;
    uint8_t : 5;

    int16_t leftTableVelocity;
    int16_t rightTableVelocity;

    int16_t effectsKnob; // Whether or not this is signed doesn't really matter, as either way it's gonna loop over when it reaches min/max
    int16_t crossfader;
    uint8_t reserved_1[6];
} __attribute__((packed)) XInputDJHTurntable_Data_t;

typedef struct {
    uint8_t bLength; // Length of this descriptor.
    uint8_t bDescriptorType; // CONFIGURATION descriptor type (USB_DESCRIPTOR_CONFIGURATION).
    uint8_t reserved[18];
} __attribute__((packed)) XBOX_ID_W_DESCRIPTOR;

typedef struct {
    uint8_t id;
    uint8_t type;
    uint8_t unknown;
    uint16_t state;
} __attribute__((packed)) XBOX_WIRELESS_HEADER;

typedef struct {
    uint8_t id;
    uint8_t type;
    uint8_t unknown1;
    // byte 0
    uint8_t vibrationLevel : 2;
    uint8_t headset : 1;
    uint8_t chatpad : 1;
    uint8_t always_0x1 : 4;
    
    // byte 1
    uint8_t unknown : 1;
    uint8_t batteryType : 2;
    uint8_t onlyMic : 1;
    uint8_t powerState : 2;
    uint8_t batteryLevel : 2;
} __attribute__((packed)) XBOX_WIRELESS_STATE;

typedef struct {
    XBOX_WIRELESS_HEADER header;
    uint8_t always_0xCC;
    uint32_t unk1;
    uint32_t deviceID;
    uint8_t type;
    uint8_t revision;
    uint8_t state[2];
    uint16_t protocol;
    uint8_t unk2[2];
    uint8_t vendorIDData[3];
    uint8_t subtype;
    uint8_t unk3[3];
} __attribute__((packed)) XBOX_WIRELESS_LINK_REPORT;

typedef struct {
    XBOX_WIRELESS_HEADER header;
    uint8_t always_0x12;
    uint16_t buttons;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint16_t leftStickX;
    uint16_t leftStickY;
    uint16_t rightStickX;
    uint16_t rightStickY;
    uint8_t leftMotor;
    uint8_t rightMotor;
    uint8_t unk[9];
} __attribute__((packed)) XBOX_WIRELESS_CAPABILITIES;