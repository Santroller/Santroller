#include "infinium_fader_device.hpp"
#include "utils.h"

InfiniumFader::InfiniumFader(uint8_t block, int8_t tx, int8_t rx, uint32_t clock)
    : m_uart(block, tx, rx, clock > 0 ? clock : 31250)
{
}

void InfiniumFader::begin()
{
    m_connected = false;
    m_last_recv = 0;
}

void InfiniumFader::end()
{
    m_connected = false;
}

void InfiniumFader::tick()
{
    while (m_uart.readable())
    {
        uint8_t c = 0;
        if (m_uart.read_uart(1, &c))
        {
            if (c >= 32 && c <= 126)
            {
                uint8_t raw = c - 32; // 0 to 94
                position = (static_cast<uint32_t>(raw) * 65535) / 94;
                m_connected = true;
                m_last_recv = millis();
            }
        }
        else
        {
            break;
        }
    }

    if (m_connected && (millis() - m_last_recv > 1000))
    {
        m_connected = false;
    }
}
