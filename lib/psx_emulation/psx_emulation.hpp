#pragma once
#include <stdint.h>
#include "pio_spi.h"

#include "spi.hpp"

#define MODE_DIGITAL 0x41
#define MODE_ANALOG  0x73
#define MODE_ANALOG_PRESSURE 0x79
#define MODE_CONFIG 0xF3
typedef enum
{
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
class PSXEmulation
{
public:
    PSXEmulation(int8_t sck, int8_t cmd, int8_t dat, uint8_t attPin, uint8_t ackPin);
    ~PSXEmulation();
    void begin();
    void end();
    void load_state(PSXEmulation* state);
    void tick();
private:
    int8_t sck;
    int8_t cmd;
    int8_t dat;
    int8_t attPin;
    int8_t ackPin;
};