#pragma once
#include <stdint.h>
#include <stdbool.h>

#define SERIAL_NUMBER_WVALUE 0x0000
#define INPUT_CAPABILITIES_WVALUE 0x0100
#define VIBRATION_CAPABILITIES_WVALUE 0x0000

typedef enum {
    OFF,
    ALL_BLINKING,
    ONE_FLASH,
    TWO_FLASH,
    THREE_FLASH,
    FOUR_FLASH,
    ONE,
    TWO,
    THREE,
    FOUR,
    ROTATING,
    BLINKING_RET,
    SLOW_BLINKING_RET,
    ALTERNATING_RET
} xinput_led_t;

typedef struct {
    uint8_t rid;
    uint8_t rsize;
    xinput_led_t led;
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
    uint8_t flags;
} __attribute__((packed)) XInputInputCapabilities_t;

typedef struct {
    uint32_t serial;
} __attribute__((packed)) XInputSerialNumber_t;

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

    // TODO: The hi-hat pedal data is probably here somewhere
    uint8_t unused1[2];
    int16_t unused2;
    uint8_t greenVelocity;
    uint8_t redVelocity;
    uint8_t yellowVelocity;
    uint8_t blueVelocity;
    uint8_t orangeVelocity;
    uint8_t kickVelocity;
    uint8_t reserved_1[6];
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
    uint16_t slider;
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

    uint16_t lowEVelocity : 7;
    uint16_t greenFret : 1;
    uint16_t aVelocity : 7;
    uint16_t redFret : 1;
    uint16_t dVelocity : 7;
    uint16_t yellowFret : 1;
    uint16_t gVelocity : 7;
    uint16_t blueFret : 1;
    uint16_t bVelocity : 7;
    uint16_t orangeFret : 1;
    uint16_t highEVelocity : 7;
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

    uint8_t key8 : 1;
    uint8_t key7 : 1;
    uint8_t key6 : 1;
    uint8_t key5 : 1;
    uint8_t key4 : 1;
    uint8_t key3 : 1;
    uint8_t key2 : 1;
    uint8_t key1 : 1;

    uint8_t key16 : 1;
    uint8_t key15 : 1;
    uint8_t key14 : 1;
    uint8_t key13 : 1;
    uint8_t key12 : 1;
    uint8_t key11 : 1;
    uint8_t key10 : 1;
    uint8_t key9 : 1;

    uint8_t key24 : 1;
    uint8_t key23 : 1;
    uint8_t key22 : 1;
    uint8_t key21 : 1;
    uint8_t key20 : 1;
    uint8_t key19 : 1;
    uint8_t key18 : 1;
    uint8_t key17 : 1;

    uint8_t velocity1 : 7;
    uint8_t key25 : 1;
    uint8_t velocity2 : 7;
    uint8_t : 1;
    uint8_t velocity3 : 7;
    uint8_t : 1;
    uint8_t velocity4 : 7;
    uint8_t : 1;
    uint8_t velocity5 : 7;
    uint8_t : 1;

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