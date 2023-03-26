#include <stdint.h>

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
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool start : 1;
    bool back : 1;
    bool leftThumbClick : 1;
    bool rightThumbClick : 1;

    bool leftShoulder : 1;
    bool rightShoulder : 1;
    bool guide : 1;
    bool : 1;

    bool a : 1;
    bool b : 1;
    bool x : 1;
    bool y : 1;

    uint8_t leftTrigger;
    uint8_t rightTrigger;
    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;
} __attribute__((packed)) XInputGamepad_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool start : 1;
    bool back : 1;
    bool leftThumbClick : 1; //pedal2
    bool padFlag : 1; //right thumb click

    bool leftShoulder : 1; // pedal1
    bool cymbalFlag : 1; // right shoulder click
    bool guide : 1;
    bool : 1;

    bool a : 1; //green
    bool b : 1; //red
    bool x : 1; //blue
    bool y : 1; //yellow

    uint8_t unused[2];
    int16_t redVelocity;
    int16_t yellowVelocity;
    int16_t blueVelocity;
    int16_t greenVelocity;
} __attribute__((packed)) XInputRockBandDrums_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool start : 1;
    bool back : 1;
    bool leftThumbClick : 1; //isGuitarHero
    bool : 1;

    bool leftShoulder : 1; // kick
    bool rightShoulder : 1; // orange
    bool guide : 1;
    bool : 1;

    bool a : 1; //green
    bool b : 1; //red
    bool x : 1; //blue
    bool y : 1; //yellow

    // TODO: The hi-hat pedal data is probably here somewhere
    uint8_t unused1[2];
    int16_t unused2;
    uint8_t greenVelocity;
    uint8_t redVelocity;
    uint8_t yellowVelocity;
    uint8_t blueVelocity;
    uint8_t orangeVelocity;
    uint8_t kickVelocity;
} __attribute__((packed)) XInputGuitarHeroDrums_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    bool dpadUp : 1; // dpadStrumUp
    bool dpadDown : 1; // dpadStrumDown
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool start : 1;
    bool back : 1;
    bool : 1;
    bool : 1;

    bool leftShoulder : 1; // orange
    bool rightShoulder : 1; // pedal
    bool guide : 1;
    bool : 1;

    bool a : 1; //green
    bool b : 1; //red
    bool x : 1; //blue
    bool y : 1; //yellow

    uint8_t accelZ;
    uint8_t accelX;
    uint16_t slider;
    int16_t unused;
    int16_t whammy;
    int16_t tilt;
} __attribute__((packed)) XInputGuitarHeroGuitar_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    bool dpadUp : 1; // dpadStrumUp
    bool dpadDown : 1; // dpadStrumDown
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool start : 1;
    bool back : 1;
    bool solo : 1; // leftThumbClick
    bool : 1;

    bool leftShoulder : 1; // orange
    bool : 1;
    bool guide : 1;
    bool : 1;

    bool a : 1; //green
    bool b : 1; //red
    bool x : 1; //blue
    bool y : 1; //yellow

    uint8_t pickup;
    uint8_t unused1;
    int16_t calibrationSensor;
    int16_t unused2;
    int16_t whammy;
    int16_t tilt;
} __attribute__((packed)) XInputRockBandGuitar_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool start : 1; // pause
    bool back : 1; // heroPower
    bool leftThumbClick : 1; // ghtv
    bool : 1;

    bool leftShoulder : 1; // white2
    bool rightShoulder : 1; // white3
    bool guide : 1;
    bool : 1;

    bool a : 1; //black1
    bool b : 1; //black2
    bool x : 1; //white1
    bool y : 1; //black3

    uint8_t unused1[2];
    int16_t unused2;

    int16_t strumBar;
    int16_t tilt;
    int16_t whammy;
} __attribute__((packed)) XInputGHLGuitar_Data_t;

typedef struct
{
    uint8_t rid;
    uint8_t rsize;
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool start : 1;
    bool back : 1;
    bool : 1;
    bool : 1;

    bool : 1;
    bool : 1;
    bool guide : 1;
    bool : 1;

    bool a : 1;
    bool b : 1;
    bool x : 1;
    bool y : 1; // euphoria

    bool leftGreen : 1;
    bool leftRed : 1;
    bool leftBlue : 1;
    uint8_t : 5;

    bool rightGreen : 1;
    bool rightRed : 1;
    bool rightBlue : 1;
    uint8_t : 5;

    int16_t leftTableVelocity;
    int16_t rightTableVelocity;

    int16_t effectsKnob;  // Whether or not this is signed doesn't really matter, as either way it's gonna loop over when it reaches min/max
    int16_t crossfader;
} __attribute__((packed)) XInputTurntable_Data_t;