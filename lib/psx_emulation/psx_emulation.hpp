#pragma once
#include <stdint.h>
#include "pio_spi.h"
#include "enums.pb.h"

#include "spi.hpp"

#define MODE_DIGITAL 0x41
#define MODE_ANALOG 0x73
#define MODE_ANALOG_PRESSURE 0x79
#define MODE_CONFIG 0xF3
typedef struct
{
    bool analog;
    uint8_t config[3];
} PsxReportFormat_t;
class PSXEmulation
{
public:
    PSXEmulation(int8_t sck, int8_t cmd, int8_t dat, uint8_t attPin, uint8_t ackPin);
    ~PSXEmulation();
    bool ready();
    void begin(SubType type);
    void end();
    void load_state(PSXEmulation *state);
    void tick();
    void sendData(uint8_t len, uint8_t *data);
    PsxReportFormat_t getReportFormat();

private:
    volatile bool sent = true;
    pio_spi_t *spi;
    int8_t sck;
    int8_t cmd;
    int8_t dat;
    int8_t attPin;
    int8_t ackPin;
};