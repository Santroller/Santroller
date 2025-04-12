#pragma once
#include <stdbool.h>
#include <stdint.h>

#define SERIAL_NUMBER_WVALUE 0x0000
#define INPUT_CAPABILITIES_WVALUE 0x0100
#define VIBRATION_CAPABILITIES_WVALUE 0x0000

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

typedef struct {
    uint8_t rid;
    uint8_t rsize;
    uint8_t led;
} __attribute__((packed)) XInputLEDReport_t;

typedef struct {
    uint8_t rid;
    uint8_t rsize;
    uint8_t unused;
    uint8_t leftRumble;
    uint8_t rightRumble;
    uint8_t unused2[3];
} __attribute__((packed)) XInputRumbleReport_t;

typedef struct {
    uint8_t rid;
    uint8_t rsize;
    uint8_t padding;
    uint8_t left_motor;
    uint8_t right_motor;
    uint8_t padding_2[3];
} __attribute__((packed)) XInputVibrationCapabilities_t;

typedef struct {
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

typedef struct {
    uint32_t serial;
} __attribute__((packed)) XInputSerialNumber_t;

typedef struct {
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

typedef struct {
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

typedef struct {
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

typedef struct {
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
    uint8_t leftThumbClick : 1;  // pedal2
    uint8_t padFlag : 1;         // right thumb click

    uint8_t leftShoulder : 1;  // pedal1
    uint8_t cymbalFlag : 1;    // right shoulder click
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t x : 1;  // blue
    uint8_t y : 1;  // yellow

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
    uint8_t leftThumbClick : 1;  // isGuitarHero
    uint8_t : 1;

    uint8_t leftShoulder : 1;   // kick
    uint8_t rightShoulder : 1;  // orange
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t x : 1;  // blue
    uint8_t y : 1;  // yellow

    uint8_t unused1[2];
    int16_t unused2;
    uint8_t greenVelocity;
    uint8_t redVelocity; // redVelocity stores the velocity for the cymbal if both cymbal and pad of the same colour get hit simultaneously
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
    uint8_t dpadUp : 1;    // dpadStrumUp
    uint8_t dpadDown : 1;  // dpadStrumDown
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t leftShoulder : 1;   // orange
    uint8_t rightShoulder : 1;  // pedal
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t x : 1;  // blue
    uint8_t y : 1;  // yellow

    uint8_t accelZ;
    uint8_t accelX;
    int16_t slider;
    int16_t unused;
    int16_t whammy;
    int16_t tilt;
    uint8_t reserved_1[6];
} __attribute__((packed)) XInputGuitarHeroGuitar_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;    // dpadStrumUp
    uint8_t dpadDown : 1;  // dpadStrumDown
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t solo : 1;  // leftThumbClick
    uint8_t : 1;

    uint8_t leftShoulder : 1;  // orange
    uint8_t : 1;
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t x : 1;  // blue
    uint8_t y : 1;  // yellow

    uint8_t pickup;
    uint8_t unused1;
    int16_t calibrationSensor;
    int16_t unused2;
    int16_t whammy;
    int16_t tilt;
    uint8_t reserved_1[6];
} __attribute__((packed)) XInputRockBandGuitar_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;    // dpadStrumUp
    uint8_t dpadDown : 1;  // dpadStrumDown
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t solo : 1;  // leftThumbClick
    uint8_t : 1;

    uint8_t leftShoulder : 1;  // orange
    uint8_t : 1;
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t x : 1;  // blue
    uint8_t y : 1;  // yellow

    uint16_t lowEFret : 5;
    uint16_t aFret : 5;
    uint16_t dFret : 5;
    uint16_t : 1;
    uint16_t gFret : 5;
    uint16_t bFret : 5;
    uint16_t highEFret : 5;
    uint16_t soloFlag : 1;

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

    uint8_t autoCal_Microphone;  // When the sensor isn't activated, this
    uint8_t autoCal_Light;       // and this just duplicate the tilt axis
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
    uint8_t dpadUp : 1;    // dpadStrumUp
    uint8_t dpadDown : 1;  // dpadStrumDown
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t solo : 1;  // leftThumbClick
    uint8_t : 1;

    uint8_t leftShoulder : 1;  // orange
    uint8_t : 1;
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t x : 1;  // blue
    uint8_t y : 1;  // yellow

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

    uint8_t pedalConnection : 1;  // If this matches PS3 MPA behavior, always 0 with the MIDI Pro Adapter
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

    uint8_t start : 1;           // start, pause
    uint8_t back : 1;            // back, heroPower
    uint8_t leftThumbClick : 1;  // leftThumbClick, ghtv
    uint8_t : 1;

    uint8_t leftShoulder : 1;   // white2 leftShoulder
    uint8_t rightShoulder : 1;  // white3 rightShoulder
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1;  // black1 a
    uint8_t b : 1;  // black2 b
    uint8_t x : 1;  // white1 x
    uint8_t y : 1;  // black3 y

    uint8_t unused1[2];

    int16_t unused2;
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
    uint8_t y : 1;  // euphoria

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

    int16_t effectsKnob;  // Whether or not this is signed doesn't really matter, as either way it's gonna loop over when it reaches min/max
    int16_t crossfader;
    uint8_t reserved_1[6];
} __attribute__((packed)) XInputTurntable_Data_t;
