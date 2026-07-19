#pragma once

#include "uart.hpp"
#include "input_enums.pb.h"
#include "commands.pb.h"

typedef struct
{
    uint8_t header = 0xa5;
    uint8_t cmd = 0x50;
    uint8_t len = 0x0F;
    uint8_t red_pad;
    uint8_t yellow_pad;
    uint8_t blue_pad;
    uint8_t green_pad;
    uint8_t orange_pad;
    uint8_t yellow_cymbal;
    uint8_t blue_cymbal;
    uint8_t green_cymbal;
    uint8_t kick1;
    uint8_t kick2;
    uint8_t footer[2] = {}; 
    uint8_t crc;                      // CRC-8/MAXIM-DOW
} __attribute__((packed)) crkd_drum_t;
class CrkdDrum
{
public:
    CrkdDrum(uint8_t block, uint8_t tx, uint8_t rx, uint32_t clock);
    void tick();
    void begin();
    void end();
    void setParam(CrkdDrumCalibrationType type, CrkdDrumAxisType axisType, uint32_t val);
    inline bool is_connected()
    {
        return m_connected && m_read_param;
    }
    
    uint8_t red_pad;
    uint8_t yellow_pad;
    uint8_t blue_pad;
    uint8_t green_pad;
    uint8_t orange_pad;
    uint8_t yellow_cymbal;
    uint8_t blue_cymbal;
    uint8_t green_cymbal;
    uint8_t kick1;
    uint8_t kick2;
    crkd_drum_t m_debounceParams;
    crkd_drum_t m_minParams;
    crkd_drum_t m_maxParams;

private:

    UARTInterface interface;
    bool m_connected = false;
    bool m_updating = false;
    crkd_drum_t m_CrkdDrum;
    long m_lastPoll;
    bool m_error = false;
    long m_lastSend;
    bool m_read_param = false;
    uint8_t m_param_cmd = 0;
};
