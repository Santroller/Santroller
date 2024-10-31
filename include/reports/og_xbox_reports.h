
#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "config.h"

typedef struct {
    uint8_t  rid; 
    uint8_t  rsize;
    uint16_t left;
    uint16_t right;
} __attribute__((packed)) OGXboxOutput_Report_t;

typedef struct {
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t start : 1;
    uint8_t back : 1;  // select
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3
    uint8_t padding;
    uint8_t a;
    uint8_t b;
    uint8_t x;
    uint8_t y;
    uint8_t rightShoulder; // black
    uint8_t leftShoulder; // white
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;
} __attribute__((packed)) OGXboxGamepad_Data_t;

typedef struct {
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t start : 1;
    uint8_t back : 1;  // select
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3
    uint8_t padding;
    uint8_t a;
    uint8_t b;
    uint8_t x;
    uint8_t y;
    uint8_t leftShoulder;
    uint8_t rightShoulder;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint16_t leftStickX;
    uint16_t leftStickY;
    uint16_t rightStickX;
    uint16_t rightStickY;
} __attribute__((packed)) OGXboxGamepadCapabilities_Data_t;


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
    uint8_t overdrive: 1;
    uint8_t padding;

    uint8_t a;  // green
    uint8_t b;  // red
    uint8_t x;  // blue
    uint8_t y;  // yellow

    uint8_t leftShoulder;  // orange
    uint8_t kickVelocity : 7;
    uint8_t kick1 : 1;
    uint8_t unused;

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

    uint8_t touchPad : 7;
    uint8_t : 1;
} __attribute__((packed)) OGXboxRockBandKeyboard_Data_t;
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
    uint8_t kick2 : 1;  // pedal2
    uint8_t padFlag : 1;         // right thumb click

    uint8_t padding;

    uint8_t green;  // green
    uint8_t red;  // red
    uint8_t blue;  // blue
    uint8_t yellow;  // yellow
    uint8_t kick1;  // pedal1
    uint8_t cymbalFlag;    // right shoulder click

    uint8_t unused[2];
    int16_t redVelocity;
    int16_t yellowVelocity;
    int16_t blueVelocity;
    int16_t greenVelocity;
} __attribute__((packed)) OGXboxRockBandDrums_Data_t;

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
    
    uint8_t padding;

    uint8_t green;  // green
    uint8_t red;  // red
    uint8_t blue;  // blue
    uint8_t yellow;  // yellow

    uint8_t kick1;   // kick
    uint8_t orange;  // orange

    uint8_t unused1[2];
    int16_t unused2;
    uint8_t greenVelocity;
    uint8_t redVelocity;
    uint8_t yellowVelocity;
    uint8_t blueVelocity;
    uint8_t orangeVelocity;
    uint8_t kickVelocity;
} __attribute__((packed)) OGXboxGuitarHeroDrums_Data_t;

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
    
    uint8_t padding;

    uint8_t green;  // green
    uint8_t red;  // red
    uint8_t blue;  // blue
    uint8_t yellow;  // yellow

    uint8_t orange;   // orange
    uint8_t kick1;  // pedal

    uint8_t accelZ;
    uint8_t accelX;
    uint16_t slider;
    int16_t unused;
    int16_t whammy;
    int16_t tilt;
} __attribute__((packed)) OGXboxGuitarHeroGuitar_Data_t;

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
    
    uint8_t padding;

    uint8_t green;  // green
    uint8_t red;  // red
    uint8_t blue;  // blue
    uint8_t yellow;  // yellow

    uint8_t orange;  // orange
    uint8_t padding2;

    uint8_t pickup;
    uint8_t unused1;
    int16_t leftStickX;
    int16_t leftStickY;
    int16_t whammy;
    int16_t tilt;
} __attribute__((packed)) OGXboxRockBandGuitar_Data_t;

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
    uint8_t ghtv : 1;  // leftThumbClick, ghtv
    uint8_t : 1;
    
    uint8_t padding;
    uint8_t black1;  // black1 a
    uint8_t black2;  // black2 b
    uint8_t white1;  // white1 x
    uint8_t black3;  // black3 y

    uint8_t white2;   // white2 leftShoulder
    uint8_t white3;  // white3 rightShoulder

    uint8_t unused1[2];
    
    int16_t leftStickX;
    int16_t leftStickY;
    int16_t tilt;
    int16_t whammy;
} __attribute__((packed)) OGXboxGHLGuitar_Data_t;

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
    uint8_t : 2;
    
    uint8_t padding;


    uint8_t a;
    uint8_t b;
    uint8_t x;
    uint8_t euphoria;  // euphoria
    uint8_t unused[2];

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
} __attribute__((packed)) OGXboxTurntable_Data_t;

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

    uint8_t padding;

    uint8_t a;
    uint8_t b;
    uint8_t x;
    uint8_t y;
    
    uint8_t : 8;
    uint8_t : 8;

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
    uint8_t kick1 : 1;

    uint8_t unused2;

    uint8_t pedalConnection : 1;
    uint8_t : 7;
} __attribute__((packed)) OGXboxRockBandProGuitar_Data_t;