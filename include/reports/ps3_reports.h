#pragma once
#include <stdint.h>

#define PS3_STICK_CENTER 0x80
#define PS3_ACCEL_CENTER 0x0200
#define PS3_REPORT_BUFFER_SIZE 48
#define GUITAR_ONE_G 40

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
    uint8_t outputType;
    uint8_t unknown1;
    uint8_t enable;  // 1 to enable, 0 to disable
    uint8_t padding[5];
} ps3_turntable_output_report_t;

typedef struct {
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

    uint8_t dpad : 4;
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
    uint16_t accelX;  // Left/right acceleration (roll)
    uint16_t accelZ;  // Forward/back acceleration (pitch)
    uint16_t accelY;  // Up/down acceleration (gravity)
    uint16_t gyro;    // Left/right instantaneous rotation (yaw)
} __attribute__((packed)) PS3Dpad_Data_t;

typedef struct {
    uint8_t reportId;
    uint8_t : 8;
    uint8_t back : 1;             // select
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3
    uint8_t start : 1;

    uint8_t dpadUp : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;

    uint8_t l2 : 1;             // l1
    uint8_t r2 : 1;             // r1
    uint8_t leftShoulder : 1;   // l1
    uint8_t rightShoulder : 1;  // r1

    uint8_t y : 1;  // triangle
    uint8_t b : 1;  // circle
    uint8_t a : 1;  // cross
    uint8_t x : 1;  // square

    uint8_t guide : 1;
    uint8_t : 7;
    uint8_t : 8;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t padding_3[4];
    uint8_t pressureDpadUp;
    uint8_t pressureDpadRight;
    uint8_t pressureDpadDown;
    uint8_t pressureDpadLeft;
    uint8_t leftTrigger;   // pressure_l2
    uint8_t rightTrigger;  // pressure_r2
    uint8_t pressureL1;
    uint8_t pressureR1;
    uint8_t pressureTriangle;
    uint8_t pressureCircle;
    uint8_t pressureCross;
    uint8_t pressureSquare;
    uint8_t padding_4[15];
    // Each of the following are 10 bits in accuracy
    // Centered/neutral state is nominally 0x0200, actual values may vary
    uint16_t accelX;  // Left/right acceleration (roll)
    uint16_t accelZ;  // Forward/back acceleration (pitch)
    uint16_t accelY;  // Up/down acceleration (gravity)
    uint16_t gyro;    // Left/right instantaneous rotation (yaw)
} __attribute__((packed)) PS3Gamepad_Data_t;

typedef struct {
    uint8_t reportId;
    uint8_t : 8;
    uint8_t back : 1;             // select
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3
    uint8_t start : 1;

    uint8_t dpadUp : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;

    uint8_t l2 : 1;             // l1
    uint8_t r2 : 1;             // r1
    uint8_t leftShoulder : 1;   // l1
    uint8_t rightShoulder : 1;  // r1

    uint8_t y : 1;  // triangle
    uint8_t b : 1;  // circle
    uint8_t a : 1;  // cross
    uint8_t x : 1;  // square

    uint8_t guide : 1;
    uint8_t : 7;
    uint8_t : 8;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t padding_3[4];
    uint8_t pressureDpadUp;
    uint8_t pressureDpadRight;
    uint8_t pressureDpadDown;
    uint8_t pressureDpadLeft;
    uint8_t leftTrigger;   // pressure_l2
    uint8_t rightTrigger;  // pressure_r2
    uint8_t whammy;
    uint8_t slider;
    uint8_t pickup;
    uint8_t tilt;
    uint8_t solo;
    uint8_t pressureSquare;
    uint8_t padding_4[15];
    // Each of the following are 10 bits in accuracy
    // Centered/neutral state is nominally 0x0200, actual values may vary
    uint16_t accelX;  // Left/right acceleration (roll)
    uint16_t accelZ;  // Forward/back acceleration (pitch)
    uint16_t accelY;  // Up/down acceleration (gravity)
    uint16_t gyro;    // Left/right instantaneous rotation (yaw)
} __attribute__((packed)) PS3GamepadGuitar_Data_t;

typedef struct {
    uint8_t reportId;
    uint8_t : 8;
    uint8_t back : 1;             // select
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3
    uint8_t start : 1;

    uint8_t b : 1; // dpadUp
    uint8_t dpadRight : 1;
    uint8_t dpadLeft : 1;
    uint8_t a : 1; // dpadLeft

    uint8_t whammy : 1;             // l2 - whammy
    uint8_t tilt : 1;             // r2 - overdrive
    uint8_t dpadUp : 1;   // l1 (leftShoulder) - strum up
    uint8_t dpadDown : 1;  // r1 (rightShoulder) - strum down

    uint8_t x : 1;  // triangle (y) - blue
    uint8_t leftShoulder : 1;  // circle (b) - orange
    uint8_t rightShoulder : 1;  // cross (a)
    uint8_t y : 1;  // square (x) - yellow

    uint8_t guide : 1;
    uint8_t : 7;
    uint8_t : 8;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t padding_3[4];
    uint8_t pressureDpadUp;
    uint8_t pressureDpadRight;
    uint8_t pressureDpadDown;
    uint8_t pressureDpadLeft;
    uint8_t leftTrigger;   // pressure_l2
    uint8_t rightTrigger;  // pressure_r2
    uint8_t pressureL1;
    uint8_t pressureR1;
    uint8_t pressureTriangle;
    uint8_t pressureCircle;
    uint8_t pressureCross;
    uint8_t pressureSquare;
    uint8_t padding_4[15];
    // Each of the following are 10 bits in accuracy
    // Centered/neutral state is nominally 0x0200, actual values may vary
    uint16_t accelX;  // Left/right acceleration (roll)
    uint16_t accelZ;  // Forward/back acceleration (pitch)
    uint16_t accelY;  // Up/down acceleration (gravity)
    uint16_t gyro;    // Left/right instantaneous rotation (yaw)
} __attribute__((packed)) PS3FestivalProGuitarLayer_Data_t;

typedef struct {
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

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
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
    uint8_t pressureDpadLeft;
    uint8_t pressureDpadRight;
    uint8_t pressureDpadUp;
    uint8_t pressureDpadDown;
    uint8_t pressureTriangle;
    uint8_t pressureCircle;
    uint8_t pressureCross;
    uint8_t pressureSquare;
    uint8_t pressureL1;
    uint8_t pressureR1;
    uint8_t leftTrigger;   //  pressure_l2
    uint8_t rightTrigger;  // pressure_r2

    // Each of the following are 10 bits in accuracy
    // Centered/neutral state is nominally 0x0200, actual values may vary
    uint16_t accelX;  // Left/right acceleration (roll)
    uint16_t accelZ;  // Forward/back acceleration (pitch)
    uint16_t accelY;  // Up/down acceleration (gravity)
    uint16_t gyro;    // Left/right instantaneous rotation (yaw)

} __attribute__((packed)) PS3SimpleGamepad_Data_t;

typedef struct
{
    uint8_t blue : 1;  // square, blue
    uint8_t green : 1;  // cross, green
    uint8_t red : 1;  // cirlce, red
    uint8_t yellow : 1;  // triangle, yellow

    uint8_t kick1 : 1;   // kick1, l1
    uint8_t kick2 : 1;  // kick2, r1
    uint8_t : 1;
    uint8_t : 1;

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t padFlag : 1;     // left stick click, l3
    uint8_t cymbalFlag : 1;  // right stick click, r3

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t : 4;

    uint8_t unused1[8];

    uint8_t yellowVelocity;
    uint8_t redVelocity;
    uint8_t greenVelocity;
    uint8_t blueVelocity;
    uint8_t blueCymbalVelocity;
    uint8_t yellowCymbalVelocity;
    uint8_t greenCymbalVelocity;
    uint8_t : 8;

    uint16_t unused3[4];
} __attribute__((packed)) PS3RockBandDrums_Data_t;

typedef struct
{
    uint8_t blue : 1;  // square, blue
    uint8_t green : 1;  // cross, green
    uint8_t red : 1;  // circle, red
    uint8_t yellow : 1;  // triangle, yellow

    uint8_t kick1 : 1;   // kick, l1
    uint8_t orange : 1;  // orange, r1
    uint8_t : 1;
    uint8_t : 1;

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t : 4;

    uint8_t unused1[8];

    uint8_t yellowVelocity;
    uint8_t redVelocity;
    uint8_t greenVelocity;
    uint8_t blueVelocity;
    uint8_t kickVelocity;
    uint8_t orangeVelocity;

    uint8_t unused2[2];
    uint16_t unused3[4];
} __attribute__((packed)) PS3GuitarHeroDrums_Data_t;


typedef struct
{
    uint8_t yellow : 1;  // triangle, yellow
    uint8_t green : 1;  // cross, green
    uint8_t red : 1;  // circle, red
    uint8_t blue : 1;  // square, blue

    uint8_t orange : 1;   // orange, l1
    uint8_t pedal : 1;  // spPedal, r1
    uint8_t : 1;
    uint8_t : 1;

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t : 4;

    uint8_t leftStickX;
    uint8_t leftStickY;
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
    uint16_t tilt;  // accelX
    uint16_t accelZ;
    uint16_t accelY;
    uint16_t unused3;
} __attribute__((packed)) PS3GuitarHeroGuitar_Data_t;

typedef struct
{
    uint8_t blue : 1;  // square, blue
    uint8_t green : 1;  // cross, green
    uint8_t red : 1;  // circle, red
    uint8_t yellow : 1;  // triangle, yellow

    uint8_t orange : 1;  // orange, l1
    uint8_t tilt : 1;          // tilt, r1
    uint8_t solo : 1;          // l2
    uint8_t : 1;               // r2

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t : 4;

    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t whammy;
    uint8_t pickup;

    uint8_t unused2[12];
    uint16_t unused3[4];
} __attribute__((packed)) PS3RockBandGuitar_Data_t;

typedef struct
{
    uint8_t x : 1;  // square, blue
    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t y : 1;  // triangle, yellow

    uint8_t : 1;  // orange, l1
    uint8_t : 1;  // tilt, r1
    uint8_t : 1;  // l2
    uint8_t : 1;  // r2

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t : 4;

    uint8_t leftStickX;
    uint8_t leftStickY;

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

    int16_t unused3[2];

    uint8_t counter;  // Unsure what this is, but this is what it's defined as in the spreadsheet linked below.
                      // No description is provided for it until more investigation can be done.

    uint8_t unused4;
} __attribute__((__packed__)) PS3RockBandProGuitar_Data_t;
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
// 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// 0x00, 0x00, 0x00
typedef struct
{
    uint8_t x : 1;  // square, blue
    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t y : 1;  // triangle, yellow

    uint8_t : 1;  // orange, l1
    uint8_t : 1;  // tilt, r1
    uint8_t : 1;  // l2
    uint8_t : 1;  // r2

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // switch capture button
    uint8_t : 2;

    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t : 4;

    uint8_t leftStickX;
    uint8_t leftStickY;

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

    uint8_t : 7;
    uint8_t overdrive : 1;

    uint8_t pedalAnalog : 7;
    uint8_t pedal : 1;

    uint8_t touchPad : 7;
    uint8_t : 1;

    uint8_t unused2[4];

    uint8_t pedalConnection : 1;  // Always 0 with the MIDI Pro Adapter
    uint8_t : 7;

    int16_t unused3[2];

    uint8_t pressCount;  // Unsure what this is, but this is what it's defined as in the spreadsheet linked below.
                         // No description is provided for it until more investigation can be done.

    uint8_t unused4;
} __attribute__((__packed__)) PS3RockBandProKeyboard_Data_t;
typedef struct
{
    uint8_t x : 1;  // square
    uint8_t a : 1;  // cross
    uint8_t b : 1;  // circle
    uint8_t euphoria : 1;  // triangle, euphoria

    uint8_t : 1;
    uint8_t : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t : 4;

    uint8_t leftStickX;
    uint8_t leftStickY;
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
    uint16_t effectsKnob;
    uint16_t crossfader;
    uint16_t rightGreen : 1;
    uint16_t rightRed : 1;
    uint16_t rightBlue : 1;
    uint16_t : 1;
    uint16_t leftGreen : 1;
    uint16_t leftRed : 1;
    uint16_t leftBlue : 1;
    uint16_t : 9;
    uint16_t unused3;
} __attribute__((packed)) PS3Turntable_Data_t;
typedef struct
{
    uint8_t white1 : 1;  // square, white1
    uint8_t black1 : 1;  // cross, black1
    uint8_t black2 : 1;  // circle, black2
    uint8_t black3 : 1;  // triangle, black3

    uint8_t white2 : 1;   // white2, l1
    uint8_t white3 : 1;  // white3, r1
    uint8_t : 1;
    uint8_t : 1;

    uint8_t back : 1;            // back, heroPower
    uint8_t start : 1;           // start, pause
    uint8_t ghtv : 1;  // leftThumbClick, ghtv
    uint8_t : 1;

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t : 4;
    uint8_t leftStickX;   // leftStickX
    uint8_t leftStickY;  // leftStickY
    uint8_t tilt2;     // rightStickX
    uint8_t whammy;    // rightStickY

    uint8_t unused3[12];

    // Reminder that this value is 10-bit in range
    uint16_t tilt;

    uint16_t unused4[3];
} __attribute__((packed)) PS3GHLGuitar_Data_t;
