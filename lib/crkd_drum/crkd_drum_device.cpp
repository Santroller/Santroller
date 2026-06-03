#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "crkd_drum_device.hpp"
#include "main.hpp"
#include "utils.h"
#include "main.hpp"
#include "hardware/irq.h"

CrkdDrum::CrkdDrum(uint8_t block, uint8_t tx, uint8_t rx, uint32_t clock) : interface(block, tx, rx, clock)
{
    interface.setup_interrupts((uint8_t *)&m_CrkdDrum, 0xA5, sizeof(m_CrkdDrum));
}
void CrkdDrum::tick()
{
    
    red_pad = m_CrkdDrum.red_pad;
    yellow_pad = m_CrkdDrum.yellow_pad;
    blue_pad = m_CrkdDrum.blue_pad;
    green_pad = m_CrkdDrum.green_pad;
    yellow_cymbal = m_CrkdDrum.yellow_cymbal;
    blue_cymbal = m_CrkdDrum.blue_cymbal;
    green_cymbal = m_CrkdDrum.green_cymbal;
    kick1 = m_CrkdDrum.kick1;
    kick2 = m_CrkdDrum.kick2;
    m_lastPoll = interface.last_read_time();
    m_connected = millis() - m_lastPoll < 10;
    // if (m_connected && millis() - m_lastSend > 500)
    // {
    //     // kick the neck over to polling at 1ms
    //     uint8_t data[] = {0xA5, 0xC1, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD5};
    //     interface.send(data, sizeof(data));
    //     m_lastSend = millis();
    // }
};
