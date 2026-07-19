#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "crkd_drum_device.hpp"
#include "main.hpp"
#include "utils.h"
#include "main.hpp"
#include "hardware/irq.h"

static uint8_t ack[] = {0xA5, 0x41, 0x08, 0x00, 0x00, 0x50, 0x00, 0xBA};
CrkdDrum::CrkdDrum(uint8_t block, uint8_t tx, uint8_t rx, uint32_t clock) : interface(block, tx, rx, clock)
{
    printf("setup drum!\r\n");
}
void CrkdDrum::begin()
{
    printf("crkd drum begin!\r\n");
    m_read_param = false;
    m_param_cmd = 0x60;
    interface.setup_interrupts((uint8_t *)&m_CrkdDrum, 0xA5, sizeof(m_CrkdDrum));
}
void CrkdDrum::end()
{
    printf("crkd drum end!\r\n");
    interface.disable_interrupts();
}
void update_crc(uint8_t *buf, uint8_t len)
{
    uint8_t i = 0, j = 0, crc = 0;
    for (j = 0; j < len - 1; j++)
    {
        crc ^= buf[j];
        for (i = 0; i < 8; i++)
        {
            if ((crc & 0x01) != 0)
            {
                crc = (uint8_t)((crc >> 1) ^ 0x8C);
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    buf[len - 1] = crc;
}
void CrkdDrum::setParam(CrkdDrumCalibrationType type, CrkdDrumAxisType axisType, uint32_t val)
{
    crkd_drum_t *data;
    switch (type)
    {
    case CrkdDrumCalibrationType::Min:
        data = &m_minParams;
        break;
    case CrkdDrumCalibrationType::Max:
        data = &m_maxParams;
        break;
    case CrkdDrumCalibrationType::Debounce:
        data = &m_debounceParams;
        break;
    }
    switch (axisType)
    {
    case CrkdDrumAxisType::CrkdRedPad:
        data->red_pad = val;
        break;
    case CrkdDrumAxisType::CrkdYellowPad:
        data->yellow_pad = val;
        break;
    case CrkdDrumAxisType::CrkdBluePad:
        data->blue_pad = val;
        break;
    case CrkdDrumAxisType::CrkdGreenPad:
        data->green_pad = val;
        break;
    case CrkdDrumAxisType::CrkdYellowCymbal:
        data->yellow_cymbal = val;
        break;
    case CrkdDrumAxisType::CrkdBlueCymbal:
        data->blue_cymbal = val;
        break;
    case CrkdDrumAxisType::CrkdGreenCymbal:
        data->green_cymbal = val;
        break;
    case CrkdDrumAxisType::CrkdKick1:
        data->kick1 = val;
        break;
    case CrkdDrumAxisType::CrkdKick2:
        data->kick2 = val;
        break;
    }
    update_crc((uint8_t *)data, sizeof(crkd_drum_t));
    interface.send((uint8_t *)data, sizeof(crkd_drum_t));
}
void CrkdDrum::tick()
{
    m_lastPoll = interface.last_read_time();
    m_connected = millis() - m_lastPoll < 20;
    if (!m_connected)
    {
        red_pad = 0;
        yellow_pad = 0;
        blue_pad = 0;
        green_pad = 0;
        orange_pad = 0;
        yellow_cymbal = 0;
        blue_cymbal = 0;
        green_cymbal = 0;
        kick1 = 0;
        kick2 = 0;
        m_read_param = false;
        m_param_cmd = 0x60;
    }
    if (m_connected && m_CrkdDrum.cmd == 0x50)
    {
        red_pad = m_CrkdDrum.red_pad;
        yellow_pad = m_CrkdDrum.yellow_pad;
        blue_pad = m_CrkdDrum.blue_pad;
        green_pad = m_CrkdDrum.green_pad;
        orange_pad = m_CrkdDrum.orange_pad;
        yellow_cymbal = m_CrkdDrum.yellow_cymbal;
        blue_cymbal = m_CrkdDrum.blue_cymbal;
        green_cymbal = m_CrkdDrum.green_cymbal;
        kick1 = m_CrkdDrum.kick1;
        kick2 = m_CrkdDrum.kick2;
        m_CrkdDrum.cmd = 0;
        if (m_param_cmd == 0)
        {
            interface.send(ack, sizeof(ack));
        }
    }

    if (m_connected && m_param_cmd && (m_CrkdDrum.cmd == m_param_cmd || m_param_cmd == 0x60))
    {
        switch (m_param_cmd)
        {
        case 0x61:
            m_debounceParams = m_CrkdDrum;
            m_debounceParams.cmd = 0x51;
            break;
        case 0x62:
            m_minParams = m_CrkdDrum;
            m_minParams.cmd = 0x52;
            break;
        case 0x63:
            m_maxParams = m_CrkdDrum;
            m_maxParams.cmd = 0x53;
            break;
        default:
            break;
        }
        m_param_cmd++;
        if (m_param_cmd <= 0x63)
        {
            uint8_t data[] = {0xA5, m_param_cmd, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};
            update_crc(data, sizeof(data));
            interface.send(data, sizeof(data));
        }
        else if (m_CrkdDrum.cmd == 0x63)
        {
            m_CrkdDrum.cmd = 0;
            m_param_cmd = 0;
            m_read_param = true;
            interface.send(ack, sizeof(ack));
        }
    }
};
