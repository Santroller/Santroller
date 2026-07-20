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
    m_parameters_read = false;
    m_param_cmd = 0x61;
    m_param_reading = false;
    m_nextParam = millis();
    m_CrkdDrum = {0};
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
    default:
        return;
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
    switch (type)
    {
    case CrkdDrumCalibrationType::Min:
        m_min_updated = true;
        break;
    case CrkdDrumCalibrationType::Max:
        m_max_updated = true;
        break;
    case CrkdDrumCalibrationType::Debounce:
        m_debounce_updated = true;
        break;
    }
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
        m_parameters_read = false;
        m_param_cmd = 0x61;
        m_param_reading = true;
    }
    if (m_connected && m_min_updated)
    {
        update_crc((uint8_t *)&m_minParams, sizeof(crkd_drum_t));
        interface.send((uint8_t *)&m_minParams, sizeof(crkd_drum_t));
        m_min_updated = false;
        m_CrkdDrum.cmd = 0;
    }
    if (m_connected && m_max_updated)
    {
        update_crc((uint8_t *)&m_maxParams, sizeof(crkd_drum_t));
        interface.send((uint8_t *)&m_maxParams, sizeof(crkd_drum_t));
        m_max_updated = false;
        m_CrkdDrum.cmd = 0;
    }
    if (m_connected && m_debounce_updated)
    {
        update_crc((uint8_t *)&m_debounceParams, sizeof(crkd_drum_t));
        interface.send((uint8_t *)&m_debounceParams, sizeof(crkd_drum_t));
        m_debounce_updated = false;
        m_CrkdDrum.cmd = 0;
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
    if (m_connected && m_CrkdDrum.cmd >= 0x51 && m_CrkdDrum.cmd <= 0x53)
    {
        m_CrkdDrum.cmd = 0;
        interface.send(ack, sizeof(ack));
    }
    if (m_connected && m_parameters_read && m_CrkdDrum.cmd >= 0x61 && m_CrkdDrum.cmd <= 0x63)
    {
        m_CrkdDrum.cmd = 0;
        interface.send(ack, sizeof(ack));
    }
    if (m_connected && m_param_cmd && !m_param_reading && millis() > m_nextParam)
    {
        uint8_t data[] = {0xA5, m_param_cmd, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};
        update_crc(data, sizeof(data));
        interface.send(data, sizeof(data));
        m_param_reading = true;
    }
    if (m_connected && m_CrkdDrum.cmd == m_param_cmd && m_param_reading)
    {
        m_param_reading = false;
        m_nextParam = millis() + 100;
        uint8_t crc = m_CrkdDrum.crc;
        update_crc((uint8_t *)&m_CrkdDrum, sizeof(m_CrkdDrum));
        if (crc == m_CrkdDrum.crc)
        {
            switch (m_param_cmd)
            {
            case 0x61:
                m_debounceParams = m_CrkdDrum;
                m_debounceParams.cmd = 0x51;
                m_param_cmd = 0x62;
                break;
            case 0x62:
                m_minParams = m_CrkdDrum;
                m_minParams.cmd = 0x52;
                m_param_cmd = 0x63;
                break;
            case 0x63:
                m_maxParams = m_CrkdDrum;
                m_maxParams.cmd = 0x53;
                m_CrkdDrum.cmd = 0;
                m_param_cmd = 0;
                m_parameters_read = true;
                interface.send(ack, sizeof(ack));
                return;
            default:
                break;
            }
        }
    }
};
