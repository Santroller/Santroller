#pragma once

#include "uart.hpp"
#include <stdint.h>
#include <stdbool.h>

class InfiniumFader
{
public:
    InfiniumFader(uint8_t block, int8_t tx, int8_t rx, uint32_t clock = 31250);
    ~InfiniumFader() {}

    void begin();
    void end();
    void tick();

    inline bool is_connected() const { return m_connected; }
    uint16_t position = 0;

private:
    UARTInterface m_uart;
    bool m_connected = false;
    uint32_t m_last_recv = 0;
};
