#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpad : 4;

    uint8_t start : 1;
    uint8_t back : 1;
    uint8_t leftThumbClick : 1;
    uint8_t rightThumbClick : 1;

    uint8_t leftShoulder : 1;
    uint8_t rightShoulder : 1;
    uint8_t guide : 1;
    uint8_t capture: 1;

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

} __attribute__((packed)) PCGamepadDpad_Data_t;

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