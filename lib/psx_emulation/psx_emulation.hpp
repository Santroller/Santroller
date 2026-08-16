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
    void data_request(uint8_t cmd);
    void transaction_ended();
    void transaction_started();
    PsxReportFormat_t getReportFormat();

private:
    bool analog = false;
    bool config = false;
    bool locked = false;
    volatile bool sent = true;
    uint8_t report_len = 2;
    uint8_t resp_42[32] = {0xff, 0xff};
    uint8_t resp_41[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    dma_channel_hw_t *write_dma;
    pio_spi_t *spi;
    volatile uint8_t dma_buf[32];
    int8_t sck;
    int8_t cmd;
    int8_t dat;
    int8_t attPin;
    int8_t ackPin;
};