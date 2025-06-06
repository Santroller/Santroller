
#include <stdbool.h>
#include <stdint.h>

#include "config.h"

// To make things easier, we use bitfields here, and then we map to a proper hat later
#define DPAD                       \
    union {                        \
        struct {                   \
            uint8_t dpadUp : 1;    \
            uint8_t dpadDown : 1;  \
            uint8_t dpadLeft : 1;  \
            uint8_t dpadRight : 1; \
            uint8_t : 4;           \
        };                         \
        uint8_t dpad;              \
    };
typedef struct {
    uint8_t reportId;
    // Button bits
    uint8_t x : 1;  // square
    uint8_t a : 1;  // cross
    uint8_t b : 1;  // circle
    uint8_t y : 1;  // triangle

    uint8_t leftShoulder : 1;   // l1
    uint8_t rightShoulder : 1;  // r1
    uint8_t l2 : 1;             // l2
    uint8_t r2 : 1;             // r2

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // switch capture button
    uint8_t : 2;

    DPAD;

    // Stick axes
    // Neutral state is 0x80
    // X axis is left at 0x00, right at 0xFF
    // Y axis is top at 0x00, bottom at 0xFF
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t leftTrigger;   // pressure_l2
    uint8_t rightTrigger;  // pressure_r2

    // Pressure axes for buttons
    // Neutral state is 0x00, max is 0xFF
    uint8_t pressureDpadUp;
    uint8_t pressureDpadRight;
    uint8_t pressureDpadLeft;
    uint8_t pressureDpadDown;
    uint8_t pressureL1;
    uint8_t pressureR1;
    uint8_t pressureTriangle;
    uint8_t pressureCircle;
    uint8_t pressureCross;
    uint8_t pressureSquare;

} __attribute__((packed)) PCGamepad_Data_t;

typedef struct {
    uint8_t reportId;
    // Button bits
    uint8_t a : 1;  // square
    uint8_t b : 1;  // cross
    uint8_t x : 1;  // circle
    uint8_t y : 1;  // triangle

    uint8_t leftShoulder : 1;   // l1
    uint8_t rightShoulder : 1;  // r1
    uint8_t l2 : 1;             // l2
    uint8_t r2 : 1;             // r2

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // switch capture button
    uint8_t : 2;

    DPAD;

    // Stick axes
    // Neutral state is 0x80
    // X axis is left at 0x00, right at 0xFF
    // Y axis is top at 0x00, bottom at 0xFF
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t leftTrigger;   // pressure_l2
    uint8_t rightTrigger;  // pressure_r2

} __attribute__((packed)) PCStageKit_Data_t;

typedef struct
{
    uint8_t reportId;

    uint8_t a : 1;  // cross
    uint8_t b : 1;  // circle
    uint8_t x : 1;  // square
    uint8_t y : 1;  // triangle

    uint8_t padFlag : 1;
    uint8_t cymbalFlag : 1;
    uint8_t leftShoulder : 1;   // kick 1
    uint8_t rightShoulder : 1;  // kick 2

    uint8_t back : 1;
    uint8_t start : 1;
    uint8_t guide : 1;
    uint8_t : 5;

    DPAD;

    uint8_t greenVelocity;
    uint8_t redVelocity;
    uint8_t yellowVelocity;
    uint8_t blueVelocity;
    uint8_t greenCymbalVelocity;
    uint8_t yellowCymbalVelocity;
    uint8_t blueCymbalVelocity;
} __attribute__((packed)) PCRockBandDrums_Data_t;

typedef struct
{
    uint8_t reportId;
    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t y : 1;  // triangle, yellow
    uint8_t x : 1;  // square, blue

    uint8_t rightShoulder : 1;  // orange, r1
    uint8_t leftShoulder : 1;   // kick, l1
    uint8_t back : 1;           // select
    uint8_t start : 1;

    uint8_t guide : 1;  // ps
    uint8_t : 7;

    DPAD;

    uint8_t greenVelocity;
    uint8_t redVelocity;
    uint8_t yellowVelocity;
    uint8_t blueVelocity;
    uint8_t orangeVelocity;
    uint8_t kickVelocity;
} __attribute__((packed)) PCGuitarHeroDrums_Data_t;

typedef struct
{
    uint8_t reportId;
    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t y : 1;  // triangle, yellow
    uint8_t x : 1;  // square, blue

    uint8_t leftShoulder : 1;   // orange, l1
    uint8_t rightShoulder : 1;  // spPedal, r1
    uint8_t back : 1;           // select
    uint8_t start : 1;

    uint8_t guide : 1;  // ps
    uint8_t : 7;

    DPAD;

    uint8_t whammy;
    uint8_t slider;
    uint8_t tilt;
} __attribute__((packed)) PCGuitarHeroGuitar_Data_t;

typedef struct
{
    uint8_t report_id;  // Always 0x1d
    uint8_t tilt;
    uint8_t accel_y;
    uint8_t padding;  // Always 0xFF
    uint8_t accel_z;

    
    uint8_t dpad : 4;
    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t x : 1;  // square, blue
    uint8_t y : 1;  // triangle, yellow

    uint8_t leftShoulder : 1;  // orange, l1
    uint8_t : 7;
} __attribute__((packed)) ArcadeGuitarHeroGuitar_Data_t;

// Festivals default mappings suck, so its easiest to set up custom mappings for that here
typedef struct
{
    uint8_t reportId;
    uint8_t rightShoulder : 1;  // a, cross, green
    uint8_t leftShoulder : 1;   // b, circle, red
    uint8_t x : 1;              // y, triangle, yellow
    uint8_t y : 1;              // x, square, blue

    uint8_t dpadUp : 1;    // leftShoulder, orange, l1
    uint8_t dpadDown : 1;  // rightShoulder, spPedal, r1
    uint8_t back : 1;      // back, select
    uint8_t start : 1;     // start

    uint8_t guide : 1;  // ps
    uint8_t : 2;
    uint8_t whammy : 1;  // l2
    uint8_t tilt : 1;    // r2
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    union {
        struct {
            uint8_t b : 1;          // dpadUp
            uint8_t dpadRight : 1;  // dpadDown
            uint8_t a : 1;          // dpadLeft
            uint8_t dpadLeft : 1;   // dpadRight
            uint8_t : 4;
        };
        uint8_t dpad;
    };

    uint8_t unused[2];
    uint8_t slider;
} __attribute__((packed)) FestivalProGuitarLayerGH_Data_t;

typedef struct
{
    uint8_t reportId;
    uint8_t rightShoulder : 1;  // a, cross, green
    uint8_t leftShoulder : 1;   // b, circle, red
    uint8_t x : 1;              // y, triangle, yellow
    uint8_t y : 1;              // x, square, blue

    uint8_t dpadUp : 1;    // leftShoulder, orange, l1
    uint8_t dpadDown : 1;  // rightShoulder
    uint8_t whammy : 1;    // l2
    uint8_t tilt : 1;      // r2
    uint8_t : 2;
    uint8_t back : 1;   // back, select
    uint8_t start : 1;  // start

    uint8_t guide : 1;  // ps
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    union {
        struct {
            uint8_t b : 1;          // dpadUp
            uint8_t dpadRight : 1;  // dpadDown
            uint8_t a : 1;          // dpadLeft
            uint8_t dpadLeft : 1;   // dpadRight
            uint8_t : 4;
        };
        uint8_t dpad;
    };

    uint8_t unused[2];
    uint8_t slider;
} __attribute__((packed)) FestivalProGuitarLayerRB_Data_t;

typedef struct
{
    uint8_t reportId;

    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t y : 1;  // triangle, yellow
    uint8_t x : 1;  // square, blue

    uint8_t leftShoulder : 1;  // orange, l1

    uint8_t soloGreen : 1;
    uint8_t soloRed : 1;
    uint8_t soloYellow : 1;
    uint8_t soloBlue : 1;

    uint8_t soloOrange : 1;
    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t guide : 1;  // ps
    uint8_t : 3;

    DPAD;

    uint8_t whammy;
    uint8_t pickup;
    uint8_t tilt;
} __attribute__((packed)) PCRockBandGuitar_Data_t;

typedef struct
{
    uint8_t reportId;

    uint8_t start : 1;
    uint8_t back : 1;  // select
    uint8_t a : 1;     // cross, green
    uint8_t b : 1;     // circle, red

    uint8_t x : 1;  // square, blue
    uint8_t y : 1;  // triangle, yellow
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;

    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t leftShoulder : 1;  // orange, l1
    uint8_t guide : 1;

    uint8_t solo : 1;
    uint8_t : 3;

    uint8_t tilt;
    uint8_t whammy;
    uint8_t pickup;

    uint8_t green : 1;
    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;
    uint8_t : 3;

    uint8_t soloGreen : 1;
    uint8_t soloRed : 1;
    uint8_t soloYellow : 1;
    uint8_t soloBlue : 1;
    uint8_t soloOrange : 1;
    uint8_t : 3;
} __attribute__((packed)) PCFortniteRockBandGuitar_Data_t;

typedef struct
{
    uint8_t reportId;

    uint8_t a : 1;  // cross
    uint8_t b : 1;  // circle
    uint8_t x : 1;  // square
    uint8_t y : 1;  // triangle, euphoria

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t guide : 1;  // ps
    uint8_t rightGreen : 1;

    uint8_t rightRed : 1;
    uint8_t rightBlue : 1;
    uint8_t leftGreen : 1;
    uint8_t leftRed : 1;

    uint8_t leftBlue : 1;
    uint8_t : 3;

    DPAD;

    uint8_t leftTableVelocity;
    uint8_t rightTableVelocity;

    uint8_t effectsKnob;
    uint8_t crossfader;
} __attribute__((packed)) PCTurntable_Data_t;

typedef struct
{
    uint8_t reportId;
    bool a : 1;  // cross, black1
    bool b : 1;  // circle, black2
    bool y : 1;  // triangle, black3

    bool x : 1;              // square, white1
    bool leftShoulder : 1;   // white2, l1
    bool rightShoulder : 1;  // white3, r1
    bool back : 1;           // back, heroPower
    bool start : 1;          // start, pause

    bool leftThumbClick : 1;  // leftThumbClick, ghtv
    bool guide : 1;           // ps
    uint8_t : 6;

    DPAD;

    uint8_t whammy;
    uint8_t tilt;
} __attribute__((packed)) PCGHLGuitar_Data_t;

typedef struct
{
    uint8_t reportId;
    uint8_t x : 1;  // square, blue
    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t y : 1;  // triangle, yellow

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t guide : 1;  // ps
    uint8_t green : 1;

    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;

    uint8_t pedal : 1;
    uint8_t : 3;

    DPAD;

    uint8_t tilt;
    uint8_t lowEFret;
    uint8_t aFret;
    uint8_t dFret;
    uint8_t gFret;
    uint8_t bFret;
    uint8_t highEFret;
    uint8_t lowEFretVelocity;
    uint8_t aFretVelocity;
    uint8_t dFretVelocity;
    uint8_t gFretVelocity;
    uint8_t bFretVelocity;
    uint8_t highEFretVelocity;
} __attribute__((__packed__)) PCRockBandProGuitar_Data_t;

typedef struct
{
    uint8_t reportId;
    uint8_t x : 1;  // square, blue
    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t y : 1;  // triangle, yellow

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t guide : 1;  // ps
    uint8_t overdrive : 1;

    uint8_t pedalDigital : 1;
    uint8_t : 7;
    DPAD;
    union {
        struct {
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
        };
        uint8_t keys[3];
    };

    uint8_t pedalAnalog;
    uint8_t touchPad;
    union {
        struct {
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
        };

        uint8_t velocities[5];
    };
} __attribute__((__packed__)) PCRockBandProKeyboard_Data_t;
typedef struct {
    uint8_t reportTypeId;  // 0x5B
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
    union {
        uint8_t noteHitRaw;
        struct {
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

typedef struct {
    uint8_t reportId;  // 0x01
    PCStageKitOutputWithoutReportId_Data_t report;
} __attribute__((packed)) PCStageKitOutput_Data_t;