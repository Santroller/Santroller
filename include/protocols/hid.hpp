#pragma once
#include <stdbool.h>
#include <stdint.h>

// To make things easier, we use bitfields here, and then we map to a proper hat later
#define DPAD                       \
    union                          \
    {                              \
        struct                     \
        {                          \
            uint8_t dpadUp : 1;    \
            uint8_t dpadDown : 1;  \
            uint8_t dpadLeft : 1;  \
            uint8_t dpadRight : 1; \
            uint8_t : 4;           \
        };                         \
        uint8_t dpad;              \
    };
typedef struct
{
    uint8_t report_id;
    DPAD;
    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t leftThumbClick : 1;
    uint8_t rightThumbClick : 1;

    uint8_t leftShoulder : 1;
    uint8_t rightShoulder : 1;
    uint8_t guide : 1;
    uint8_t capture : 1;

    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1;

    uint8_t l2 : 1; // l2
    uint8_t r2 : 1; // r2
    uint8_t z : 1;
    uint8_t : 1;

    uint8_t leftTrigger;  // pressure_l2
    uint8_t rightTrigger; // pressure_r2
    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;
    uint8_t reserved[10];

} __attribute__((packed)) PCGamepad_Data_t;

typedef struct
{
    uint8_t report_id;
    DPAD;
    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t leftThumbClick : 1;
    uint8_t rightThumbClick : 1;

    uint8_t leftShoulder : 1;
    uint8_t rightShoulder : 1;
    uint8_t guide : 1;
    uint8_t capture : 1;

    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1;

    uint8_t l2 : 1; // l2
    uint8_t r2 : 1; // r2
    uint8_t z : 1;
    uint8_t : 1;

    uint8_t leftTrigger;  // pressure_l2
    uint8_t rightTrigger; // pressure_r2
    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;
    uint8_t reserved[10];

} __attribute__((packed)) PCStageKit_Data_t;

typedef struct
{
    uint8_t report_id;
    DPAD;

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

    uint8_t : 4;

    uint8_t unused[2];
    int16_t redVelocity;
    int16_t yellowVelocity;
    int16_t blueVelocity;
    int16_t greenVelocity;
    uint8_t reserved[10];
} __attribute__((packed)) PCRockBandDrums_Data_t;

typedef struct
{
    uint8_t report_id;
    DPAD;
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

    uint8_t : 4;

    uint8_t unused1[2];
    int16_t leftStickX;
    union
    {
        struct
        {
            uint8_t greenVelocity;
            uint8_t redVelocity;
        };
        int16_t leftStickY;
    };
    uint8_t yellowVelocity;
    uint8_t blueVelocity;
    uint8_t orangeVelocity;
    uint8_t kickVelocity;
    uint8_t midiPacket[10];
} __attribute__((packed)) PCGuitarHeroDrums_Data_t;

typedef struct
{
    uint8_t report_id;
    DPAD;
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

    uint8_t : 4;

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
    int16_t unused;
    int16_t whammy;
    int16_t tilt;
    uint8_t reserved[10];
} __attribute__((packed)) PCGuitarHeroGuitar_Data_t;

typedef struct
{
    uint8_t report_id;
    DPAD;

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

    uint8_t : 4;

    uint8_t pickup;
    uint8_t unused1;
    int16_t calibrationSensor;
    int16_t unused2;
    int16_t whammy;
    int16_t tilt;
    uint8_t reserved[10];
} __attribute__((packed)) PCRockBandGuitar_Data_t;

typedef struct
{
    uint8_t report_id;
    DPAD;

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

    uint8_t : 4;

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
    uint8_t reserved[10];
} __attribute__((packed)) PCDJHTurntable_Data_t;

typedef struct
{
    uint8_t report_id;
    DPAD;
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
    uint8_t : 4;

    uint8_t unused1[2];

    int16_t leftStickX;
    int16_t strumBar;
    int16_t tilt;
    int16_t whammy;
    uint8_t reserved[10];
} __attribute__((packed)) PCGHLGuitar_Data_t;

typedef struct
{
    uint8_t report_id;
    DPAD;
    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t : 1;
    uint8_t : 1;
    uint8_t guide : 1;
    uint8_t : 1;

    uint8_t a : 1; // green
    uint8_t b : 1; // red
    uint8_t x : 1; // blue
    uint8_t y : 1; // yellow
    uint8_t : 4;

    uint16_t lowEFret : 5;
    uint16_t aFret : 5;
    uint16_t dFret : 5;
    uint16_t : 1;
    uint16_t gFret : 5;
    uint16_t bFret : 5;
    uint16_t highEFret : 5;
    uint16_t solo : 1;
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
    uint8_t reserved[4];
} __attribute__((__packed__)) PCRockBandProGuitar_Data_t;

typedef struct
{
    uint8_t report_id;
    DPAD;

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
    uint8_t : 4;

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
    uint8_t reserved[4];
} __attribute__((__packed__)) PCRockBandProKeyboard_Data_t;

typedef struct
{
    uint8_t report_id;
    uint8_t reportTypeId; // 0x5B
    uint8_t stageKitStrobe : 7;
    uint8_t stageKitFog : 1;
    uint8_t stageKitBlue;
    uint8_t stageKitGreen;
    uint8_t stageKitYellow;
    uint8_t stageKitRed;
    uint8_t multiplier;
    uint8_t starPowerState;
    uint8_t starPowerActive : 1;
    uint8_t soloActive : 1;
    uint8_t noteMiss : 1;
    uint8_t : 5;
    union
    {
        uint8_t noteHitRaw;
        struct
        {
#if DEVICE_TYPE == ROCK_BAND_GUITAR || DEVICE_TYPE == GUITAR_HERO_GUITAR
            bool openHit : 1;
            bool greenHit : 1;
            bool redHit : 1;
            bool yellowHit : 1;

            bool blueHit : 1;
            bool orangeHit : 1;
            uint8_t : 2;
#elif DEVICE_TYPE == LIVE_GUITAR
            bool openHit : 1;
            bool black1Hit : 1;
            bool black2Hit : 1;
            bool black3Hit : 1;

            bool white1Hit : 1;
            bool white2Hit : 1;
            bool white3Hit : 1;
            uint8_t : 1;
#elif DEVICE_TYPE == GUITAR_HERO_DRUMS
            bool kickHit : 1;
            bool redPadHit : 1;
            bool yellowCymbalHit : 1;
            bool bluePadHit : 1;

            bool orangeCymbalHit : 1;
            bool greenPadHit : 1;
            uint8_t : 2;
#elif DEVICE_TYPE == ROCK_BAND_DRUMS
            bool kickHit : 1;
            bool redPadHit : 1;
            bool yellowPadHit : 1;
            bool bluePadHit : 1;

            bool greenPadHit : 1;
            bool yellowCymbalHit : 1;
            bool blueCymbalHit : 1;
            bool greenCymbalHit : 1;
#elif DEVICE_TYPE == DJ_HERO_TURNTABLE
            bool leftScratchHit : 1;
            bool leftGreenHit : 1;
            bool leftRedHit : 1;
            bool leftBlueHit : 1;

            bool rightScratchHit : 1;
            bool rightGreenHit : 1;
            bool rightRedHit : 1;
            bool rightBlueHit : 1;

            uint8_t euphoriaBrightness;
#endif
        };
    };
} __attribute__((packed)) PCStageKitOutputWithoutReportId_Data_t;

typedef struct
{
    uint8_t reportId; // 0x01
    PCStageKitOutputWithoutReportId_Data_t report;
} __attribute__((packed)) PCStageKitOutput_Data_t;


typedef struct
{
    uint8_t always_1d;  // Always 0x1d
    uint8_t unk1;
    uint8_t unk2;
    uint8_t always_ff;  // Always 0xFF
    uint8_t tilt;

    union {
        struct {
            uint8_t dpadUp : 1;
            uint8_t dpadDown : 1;
            uint8_t dpadLeft : 1;
            uint8_t dpadRight : 1;
            uint8_t a : 1;  // cross, green
            uint8_t b : 1;  // circle, red
            uint8_t x : 1;  // square, blue
            uint8_t y : 1;  // triangle, yellow
        };
        struct {
            uint8_t dpad : 4;
            uint8_t : 4;
        };
    };

    uint8_t leftShoulder : 1;  // orange, l1
    uint8_t : 3;
    uint8_t side : 4;  // 1 for left, 2 for right
} __attribute__((packed)) ArcadeGuitarHeroGuitar_Data_t;