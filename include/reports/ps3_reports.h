#include <stdint.h>

#include "reports/xbox_one_reports.h"

typedef struct {
    uint8_t padding;
    uint8_t right_duration;   /* Right motor duration (0xff means forever) */
    uint8_t right_motor_on;   /* Right (small) motor on/off, only supports values of 0 or 1 (off/on) */
    uint8_t left_duration;    /* Left motor duration (0xff means forever) */
    uint8_t left_motor_force; /* left (large) motor, supports force values from 0 to 255 */
} __attribute__((packed)) ps3_rumble_t;

typedef struct {
    uint8_t time_enabled; /* the total time the led is active (0xff means forever) */
    uint8_t duty_length;  /* how long a cycle is in deciseconds (0 means "really fast") */
    uint8_t enabled;
    uint8_t duty_off; /* % of duty_length the led is off (0xff means 100%) */
    uint8_t duty_on;  /* % of duty_length the led is on (0xff mean 100%) */
} __attribute__((packed)) ps3_led_t;

typedef struct {
    uint8_t report_id;
    ps3_rumble_t rumble;
    uint8_t padding[4];
    uint8_t leds_bitmap; /* bitmap of enabled LEDs: LED_1 = 0x02, LED_2 = 0x04, ... */
    ps3_led_t led[4];    /* LEDx at (4 - x) */
    ps3_led_t _reserved; /* LED5, not actually soldered */
} __attribute__((packed)) ps3_output_report;

typedef struct {
    uint16_t buttons;
    uint8_t dpad;
} __attribute__((packed)) PS3Dpad_Data_t;

typedef struct {
    // Button bits
    bool x : 1;
    bool b : 1;
    bool a : 1;
    bool y : 1;

    bool leftShoulder : 1;   // l1
    bool rightShoulder : 1;  // r1
    bool l2 : 1;             // l2
    bool r2 : 1;             // r2

    bool back : 1;  // select
    bool start : 1;
    bool leftThumbClick : 1;   // l3
    bool rightThumbClick : 1;  // r3

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    // Stick axes
    // Neutral state is 0x80
    // X axis is left at 0x00, right at 0xFF
    // Y axis is top at 0x00, bottom at 0xFF
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;

    uint8_t unused2[12];
    int16_t unused3[4];

} __attribute__((packed)) SwitchGamepad_Data_t;

typedef struct {
    // Button bits
    bool x : 1;  // square
    bool a : 1;  // cross
    bool b : 1;  // circle
    bool y : 1;  // triangle

    bool leftShoulder : 1;   // l1
    bool rightShoulder : 1;  // r1
    bool l2 : 1;             // l2
    bool r2 : 1;             // r2

    bool back : 1;  // select
    bool start : 1;
    bool leftThumbClick : 1;   // l3
    bool rightThumbClick : 1;  // r3

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    // Stick axes
    // Neutral state is 0x80
    // X axis is left at 0x00, right at 0xFF
    // Y axis is top at 0x00, bottom at 0xFF
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;

    // Pressure axes for buttons
    // Neutral state is 0x00, max is 0xFF
    uint8_t pressureDpadUp;
    uint8_t pressureDpadRight;
    uint8_t pressureDpadLeft;
    uint8_t pressureDpadDown;
    uint8_t leftTrigger;   // pressure_l2
    uint8_t rightTrigger;  // pressure_r2
    uint8_t pressureL1;
    uint8_t pressureR1;
    uint8_t pressureTriangle;
    uint8_t pressureCircle;
    uint8_t pressureCross;
    uint8_t pressureSquare;

    // Each of the following are 10 bits in accuracy
    // Centered/neutral state is nominally 0x0200, actual values may vary
    int16_t accelX;  // Left/right acceleration (roll)
    int16_t accelZ;  // Forward/back acceleration (pitch)
    int16_t accelY;  // Up/down acceleration (gravity)
    int16_t gyro;    // Left/right instantaneous rotation (yaw)

} __attribute__((packed)) PS3Gamepad_Data_t;

typedef struct
{
    bool x : 1;  // square, blue
    bool a : 1;  // cross, green
    bool b : 1;  // cirlce, red
    bool y : 1;  // triangle, yellow

    bool leftShoulder : 1;   // kick1, l1
    bool rightShoulder : 1;  // kick2, r1
    bool : 1;
    bool : 1;

    bool back : 1;  // select
    bool start : 1;
    bool leftThumbClick : 1;   // pad, l3
    bool rightThumbClick : 1;  // cymbal, r3

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    uint8_t unused1[8];

    uint8_t yellowVelocity;
    uint8_t redVelocity;
    uint8_t greenVelocity;
    uint8_t blueVelocity;

    uint8_t unused2[4];
    int16_t unused3[4];
} __attribute__((packed)) PS3RockBandDrums_Data_t;

typedef struct
{
    bool x : 1;  // square, blue
    bool a : 1;  // cross, green
    bool b : 1;  // circle, red
    bool y : 1;  // triangle, yellow

    bool leftShoulder : 1;   // kick, l1
    bool rightShoulder : 1;  // orange, r1
    bool : 1;
    bool : 1;

    bool back : 1;  // select
    bool start : 1;
    bool : 1;
    bool : 1;

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    uint8_t unused1[8];

    uint8_t yellowVelocity;
    uint8_t redVelocity;
    uint8_t greenVelocity;
    uint8_t blueVelocity;
    uint8_t kickVelocity;
    uint8_t orangeVelocity;

    uint8_t unused2[2];
    int16_t unused3[4];
} __attribute__((packed)) PS3GuitarHeroDrums_Data_t;

typedef struct
{
    bool y : 1;  // triangle, yellow
    bool a : 1;  // cross, green
    bool b : 1;  // circle, red
    bool x : 1;  // square, blue

    bool leftShoulder : 1;   // orange, l1
    bool rightShoulder : 1;  // spPedal, r1
    bool : 1;
    bool : 1;

    bool back : 1;  // select
    bool start : 1;
    bool : 1;
    bool : 1;

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    uint8_t tilt_pc;
    uint8_t unused1;
    uint8_t whammy;
    uint8_t slider;

    uint8_t pressure_dpadRight_yellow;  // yellow
    uint8_t pressure_dpadLeft;
    uint8_t pressure_dpadUp_green;     // green
    uint8_t pressure_dpadDown_orange;  // orange
    uint8_t pressure_blue;
    uint8_t pressure_red;
    uint8_t unused2[6];

    // Reminder that these values are 10-bit in range
    int16_t tilt;  // accelX
    int16_t accelZ;
    int16_t accelY;
    int16_t unused3;
} __attribute__((packed)) PS3GuitarHeroGuitar_Data_t;

typedef struct
{
    bool blue : 1;
    bool green : 1;
    bool red : 1;
    bool yellow : 1;

    bool leftShoulder : 1;  // orange, l1
    bool tilt : 1;          // tilt, r1
    bool solo : 1;          // l2
    bool : 1;               // r2

    bool back : 1;  // select
    bool start : 1;
    bool : 1;
    bool : 1;

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    uint8_t tilt_pc;
    uint8_t unused1;
    uint8_t whammy;
    uint8_t pickup;

    uint8_t unused2[12];
    int16_t unused3[4];
} __attribute__((packed)) PS3RockBandGuitar_Data_t;

typedef struct
{
    bool x : 1;  // square
    bool a : 1;  // cross
    bool b : 1;  // circle
    bool y : 1;  // triangle, euphoria

    bool : 1;
    bool : 1;
    bool : 1;
    bool : 1;

    bool back : 1;  // select
    bool start : 1;
    bool : 1;
    bool : 1;

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    uint8_t unused1[2];
    uint8_t leftTableVelocity;
    uint8_t rightTableVelocity;

    uint8_t pressure_dpadRight_square;
    uint8_t pressure_dpadLeft;
    uint8_t pressure_dpadUp_cross;
    uint8_t pressure_dpadDown;
    uint8_t pressure_triangle;
    uint8_t pressure_circle;
    uint8_t unused2[6];

    // Reminder that these values are 10-bit in range
    int16_t effectsKnob;
    int16_t crossfader;
    bool : 6;
    bool tableNeutral : 1;
    bool : 3;
    bool leftBlue : 1;
    bool leftRed : 1;
    bool leftGreen : 1;
    bool rightBlue : 1;
    bool rightRed : 1;
    bool rightGreen : 1;
    int16_t unused3;
} __attribute__((packed)) PS3Turntable_Data_t;

typedef struct
{
    bool x : 1;  // square, white1
    bool a : 1;  // cross, black1
    bool b : 1;  // circle, black2
    bool y : 1;  // triangle, black3

    bool leftShoulder : 1;   // white2, l1
    bool rightShoulder : 1;  // white3, r1
    bool : 1;
    bool : 1;

    bool back : 1;            // heroPower
    bool start : 1;           // pause
    bool leftThumbClick : 1;  // ghtv
    bool : 1;

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    uint8_t tilt_pc;
    uint8_t strumBar;
    uint8_t unused2;
    uint8_t whammy;

    uint8_t unused3[12];

    // Reminder that this value is 10-bit in range
    int16_t tilt;

    int16_t unused4[3];
} __attribute__((packed)) PS3GHLGuitar_Data_t;