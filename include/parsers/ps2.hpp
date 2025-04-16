#pragma once
#include "base.hpp"
typedef enum {
    PSX_UNKNOWN_CONTROLLER = 0,
    PSX_DIGITAL,
    PSX_DUALSHOCK_1_CONTROLLER,
    PSX_DUALSHOCK_2_CONTROLLER,
    PSX_GUITAR_HERO_CONTROLLER,
    PSX_NEGCON,
    PSX_JOGCON,
    PSX_GUNCON,
    PSX_FLIGHTSTICK,
    PSX_MOUSE,
    PSX_NO_DEVICE
} PsxControllerType_t;
class PS2Parser : public Parser {
   public:
    virtual void parse(uint8_t* report, uint8_t len, san_base_t* data);
    virtual int build(san_base_t* data, uint8_t* output);

    bool hiRes = false;
    bool hasTapBar = false;
    PsxControllerType_t mType = PSX_NO_DEVICE;

};