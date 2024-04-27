#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "reports/pc_reports.h"
#include "reports/ps3_reports.h"
#include "reports/ps4_reports.h"
#include "reports/xbox_one_reports.h"
#include "reports/xinput_reports.h"
#include "reports/og_xbox_reports.h"
#include "reports/raphnet_reports.h"

#define SIMULTANEOUS_KEYS 6
#define NKRO_KEYS ((0x73 / 8) + 1)
#define SIMULTANEOUS_MIDI 12

typedef struct {
    // TMIDI_EventPacket_t midi[SIMULTANEOUS_MIDI];
} USB_MIDI_Data_t;

typedef struct {
    uint8_t rid;
    bool left : 1;
    bool right : 1;
    bool middle : 1;
    uint8_t : 5;
    int8_t x;       /**< Current delta X movement of the mouse. */
    int8_t y;       /**< Current delta Y movement on the mouse. */
    int8_t scrollY; /** Current scroll Y delta movement on the mouse */
    int8_t scrollX; /** Current scroll X delta movement on the mouse */
} __attribute__((packed)) USB_Mouse_Data_t;
typedef struct {
    uint8_t rid;
    bool mediaNextTrack : 1;
    bool mediaPreviousTrack : 1;
    bool mediaStop : 1;
    bool mediaPlayPause : 1;
    bool volumeMute : 1;
    bool volumeUp : 1;
    bool volumeDown : 1;
    bool : 1;
} USB_ConsumerControl_Data_t;

typedef struct {
    uint8_t rid;
    bool leftCtrl : 1;
    bool leftShift : 1;
    bool leftAlt : 1;
    bool lWin : 1;
    bool rightCtrl : 1;
    bool rightShift : 1;
    bool rightAlt : 1;
    bool rWin : 1;
    uint8_t : 4;
    bool a : 1;
    bool b : 1;
    bool c : 1;
    bool d : 1;
    bool e : 1;
    bool f : 1;
    bool g : 1;
    bool h : 1;
    bool i : 1;
    bool j : 1;
    bool k : 1;
    bool l : 1;
    bool m : 1;
    bool n : 1;
    bool o : 1;
    bool p : 1;
    bool q : 1;
    bool r : 1;
    bool s : 1;
    bool t : 1;
    bool u : 1;
    bool v : 1;
    bool w : 1;
    bool x : 1;
    bool y : 1;
    bool z : 1;
    bool d1 : 1;
    bool d2 : 1;
    bool d3 : 1;
    bool d4 : 1;
    bool d5 : 1;
    bool d6 : 1;
    bool d7 : 1;
    bool d8 : 1;
    bool d9 : 1;
    bool d0 : 1;
    bool enter : 1;
    bool escape : 1;
    bool back : 1;
    bool tab : 1;
    bool space : 1;
    bool oemMinus : 1;
    bool oemPlus : 1;
    bool oemOpenBrackets : 1;
    bool oemCloseBrackets : 1;
    bool oemPipe : 1;
    bool : 1;  // Unused ansi pipe
    bool oemSemicolon : 1;
    bool oemQuotes : 1;
    bool oemTilde : 1;
    bool oemComma : 1;
    bool oemPeriod : 1;
    bool oemQuestion : 1;
    bool capsLock : 1;
    bool f1 : 1;
    bool f2 : 1;
    bool f3 : 1;
    bool f4 : 1;
    bool f5 : 1;
    bool f6 : 1;
    bool f7 : 1;
    bool f8 : 1;
    bool f9 : 1;
    bool f10 : 1;
    bool f11 : 1;
    bool f12 : 1;
    bool printScreen : 1;
    bool scroll : 1;
    bool pause : 1;
    bool insert : 1;
    bool home : 1;
    bool pageUp : 1;
    bool del : 1;
    bool end : 1;
    bool pageDown : 1;
    bool right : 1;
    bool left : 1;
    bool down : 1;
    bool up : 1;
    bool numLock : 1;
    bool divide : 1;
    bool multiply : 1;
    bool subtract : 1;
    bool add : 1;
    bool : 1;  // numpad enter
    bool numPad1 : 1;
    bool numPad2 : 1;
    bool numPad3 : 1;
    bool numPad4 : 1;
    bool numPad5 : 1;
    bool numPad6 : 1;
    bool numPad7 : 1;
    bool numPad8 : 1;
    bool numPad9 : 1;
    bool numPad0 : 1;
    bool decimal : 1;
    bool : 1;
    bool apps : 1;
    bool : 1;
    bool : 1;
    bool f13 : 1;
    bool f14 : 1;
    bool f15 : 1;
    bool f16 : 1;
    bool f17 : 1;
    bool f18 : 1;
    bool f19 : 1;
    bool f20 : 1;
    bool f21 : 1;
    bool f22 : 1;
    bool f23 : 1;
    bool f24 : 1;
    uint8_t : 4;

} __attribute__((packed)) USB_NKRO_Data_t;

typedef struct {
    uint8_t rid;
    bool leftCtrl : 1;
    bool leftShift : 1;
    bool leftAlt : 1;
    bool lWin : 1;
    bool rightCtrl : 1;
    bool rightShift : 1;
    bool rightAlt : 1;
    bool rWin : 1;
    uint8_t Reserved;                   /**< Reserved for OEM use, always set to 0. */
    uint8_t KeyCode[SIMULTANEOUS_KEYS]; /**< Key codes of the currently pressed
                                           keys. */
} __attribute__((packed)) USB_6KRO_Data_t;

typedef union {
#ifdef TICK_SIXKRO
    USB_6KRO_Data_t keyboard;
#else
    USB_NKRO_Data_t keyboard;
#endif
    USB_MIDI_Data_t midi;
    uint8_t raw[64];
    USB_Mouse_Data_t mouse;
} USB_Report_Data_t;

typedef union {
    struct {
        bool x : 1;  // square
        bool a : 1;  // cross
        bool b : 1;  // circle
        bool y : 1;  // triangle

        bool leftShoulder : 1;   // l1
        bool rightShoulder : 1;  // r1
    };
    uint8_t val;
} Buffer_Report_t;

typedef struct {
    uint8_t x : 1;              // square
    uint8_t a : 1;              // cross
    uint8_t b : 1;              // circle
    uint8_t y : 1;              // triangle
    uint8_t leftShoulder : 1;   // l1
    uint8_t rightShoulder : 1;  // r1
    uint8_t kick1 : 1;
    uint8_t kick2 : 1;

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3
    uint8_t guide : 1;            // ps
    uint8_t capture : 1;          // switch capture button
    uint8_t leftBlue : 1;
    uint8_t leftRed : 1;

    uint8_t leftGreen : 1;
    uint8_t rightBlue : 1;
    uint8_t rightRed : 1;
    uint8_t rightGreen : 1;
    uint8_t soloGreen : 1;
    uint8_t soloRed : 1;
    uint8_t soloYellow : 1;
    uint8_t soloBlue : 1;

    uint8_t soloOrange : 1;
    uint8_t green : 1;
    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;

    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t yellowCymbal : 1;
    uint8_t blueCymbal : 1;
    uint8_t greenCymbal : 1;
    uint8_t : 3;

    uint16_t leftTrigger;
    uint16_t rightTrigger;

    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;
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
    uint8_t redVelocity;
    uint8_t yellowVelocity;
    uint8_t blueVelocity;
    uint8_t greenVelocity;
    uint8_t orangeVelocity;
    uint8_t blueCymbalVelocity;
    uint8_t yellowCymbalVelocity;
    uint8_t greenCymbalVelocity;
    uint8_t kickVelocity;
    uint8_t whammy;
    uint8_t pickup;
    int16_t tilt;
    uint8_t slider;
    int16_t leftTableVelocity;
    int16_t rightTableVelocity;
    int16_t effectsKnob;
    int16_t crossfader;
    uint16_t accelX;
    uint16_t accelZ;
    uint16_t accelY;
    uint16_t gyro;
    union {
        struct {
            uint16_t genericButton1 : 1;
            uint16_t genericButton2 : 1;
            uint16_t genericButton3 : 1;
            uint16_t genericButton4 : 1;
            uint16_t genericButton5 : 1;
            uint16_t genericButton6 : 1;
            uint16_t genericButton7 : 1;
            uint16_t genericButton8 : 1;
            uint16_t genericButton9 : 1;
            uint16_t genericButton10 : 1;
            uint16_t genericButton11 : 1;
            uint16_t genericButton12 : 1;
            uint16_t genericButton13 : 1;
            uint16_t genericButton14 : 1;
            uint16_t genericButton15 : 1;
            uint16_t genericButton16 : 1;
        };
        uint16_t genericButtons;
    };
    uint16_t genericX;
    uint16_t genericY;
    uint16_t genericZ;
    uint16_t genericRX;
    uint16_t genericRY;
    uint16_t genericRZ;
    uint16_t genericSlider;
} __attribute__((packed)) USB_Host_Data_t;