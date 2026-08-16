#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <protocols/ps2.hpp>
#include <pico/unique_id.h>
#include "emulation/ps2_emulation.hpp"
#include "utils.h"

Ps2EmulationDeviceInstance::Ps2EmulationDeviceInstance(proto_PSXEmulationDevice device) : m_device(device), m_controller(device.clockPin, device.commandPin, device.dataPin, device.attentionPin, device.acknowledgePin)
{
}
Ps2EmulationDeviceInstance::~Ps2EmulationDeviceInstance()
{
}
void Ps2EmulationDeviceInstance::initialize()
{
    m_controller.begin(subtype);
    switch (subtype)
    {
    case Gamepad:
    {
        PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)initialReport;
        memset(report, 0, sizeof(PS2Gamepad_Data_t));
        m_size = sizeof(PS2Gamepad_Data_t);
        report->leftStickX = PS3_STICK_CENTER;
        report->leftStickY = PS3_STICK_CENTER;
        report->rightStickX = PS3_STICK_CENTER;
        report->rightStickY = PS3_STICK_CENTER;

        break;
    }
    case GuitarHeroGuitar:
    {

        PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)initialReport;
        memset(report, 0, sizeof(PS2GuitarHeroGuitar_Data_t));
        m_size = sizeof(PS2GuitarHeroGuitar_Data_t);
        break;
    }
    }
}
void Ps2EmulationDeviceInstance::process()
{
    // TODO: do we need to limit poll rate with this
    // m_device->ready();
    memcpy(m_buffer, initialReport, sizeof(initialReport));
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(false, false);
            mapping->update_ps2(m_buffer);
        }
        for (const auto &led : profile->leds)
        {
            led->update(false, false);
        }
    }
    m_buffer[0] = ~m_buffer[0];
    m_buffer[1] = ~m_buffer[1];
    if (subtype == Gamepad)
    {
        PsxReportFormat_t format = m_controller.getReportFormat();
        if (!format.analog)
        {
            m_controller.sendData(2, m_buffer);
            return;
        }
        // PS2 allows specifying what data is sent, so handle that
        uint8_t current = 0;
        for (int i = 0; i < 24; i++)
        {
            if (format.config[i / 8] & (1 << (i % 8)))
            {
                m_buffer_formatted[current++] = m_buffer[i];
            }
        }
        m_controller.sendData(current, m_buffer_formatted);
        return;
    }
    // TODO: handle vibration and player leds
    m_controller.sendData(m_size, m_buffer);
}