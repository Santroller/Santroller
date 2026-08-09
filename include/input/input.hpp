#pragma once
#include <stdint.h>
#include "input_enums.pb.h"
class Input
{
public:
    Input(){}
    virtual ~Input(){}
    virtual bool tickDigital() = 0;
    virtual uint16_t tickAnalog() = 0;
    virtual void setup() = 0;
};


class DrumState {
public:
    RockBandDrumsAxisType lastDrum;
    uint32_t redPad;
    uint32_t yellowCymbal;
    uint32_t yellowPad;
    uint32_t blueCymbal;
    uint32_t bluePad;
    uint32_t greenCymbal;
    uint32_t greenPad;
};