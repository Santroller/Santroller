#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "reports/ps3_reports.h"
#include "reports/ps4_reports.h"
#include "reports/xbox_one_reports.h"
#include "reports/xinput_reports.h"

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
    int8_t X;       /**< Current delta X movement of the mouse. */
    int8_t Y;       /**< Current delta Y movement on the mouse. */
    int8_t ScrollY; /** Current scroll Y delta movement on the mouse */
    int8_t ScrollX; /** Current scroll X delta movement on the mouse */
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

typedef union {
    USB_NKRO_Data_t keyboard;
    USB_MIDI_Data_t midi;
    uint8_t raw[64];
    USB_Mouse_Data_t mouse;
} USB_Report_Data_t;

typedef struct {
    bool x : 1;  // square
    bool a : 1;  // cross
    bool b : 1;  // circle
    bool y : 1;  // triangle

    bool leftShoulder : 1;   // l1
    bool rightShoulder : 1;  // r1
    bool kick1 : 1;
    bool kick2 : 1;

    bool back : 1;  // select
    bool start : 1;
    bool leftThumbClick : 1;   // l3
    bool rightThumbClick : 1;  // r3

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    bool solo : 1;
    bool leftBlue : 1;

    bool leftRed : 1;
    bool leftGreen : 1;
    bool rightBlue : 1;
    bool rightRed : 1;

    bool rightGreen : 1;
    bool tableNeutral : 1;
    bool soloGreen : 1;
    bool soloRed : 1;

    bool soloYellow : 1;
    bool soloBlue : 1;
    bool soloOrange : 1;
    bool green : 1;

    bool red : 1;
    bool yellow : 1;
    bool blue : 1;
    bool orange : 1;

    bool yellowCymbal : 1;
    bool blueCymbal : 1;
    bool greenCymbal : 1;

    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;

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
    int16_t whammy;
    int16_t tilt;
    int16_t pickup;
    int16_t slider;
    int16_t leftTableVelocity;
    int16_t rightTableVelocity;
    int16_t effectsKnob;
    int16_t crossfader;
    uint16_t accelX;
    uint16_t accelZ;
    uint16_t accelY;
    uint16_t gyro;
} USB_Host_Data_t;