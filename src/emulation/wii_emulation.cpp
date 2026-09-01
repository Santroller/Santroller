#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <protocols/wii.hpp>
#include <pico/unique_id.h>
#include "emulation/wii_emulation.hpp"
#include "utils.h"

WiiEmulationDeviceInstance::WiiEmulationDeviceInstance(proto_WiiEmulationDevice device) : m_device(device), m_controller(device.i2c.block, device.i2c.sda, device.i2c.scl)
{
}
WiiEmulationDeviceInstance::~WiiEmulationDeviceInstance()
{
}
void WiiEmulationDeviceInstance::initialize()
{
    m_controller.begin(subtype);
    memset(initialReport, 0, sizeof(initialReport));
    if (subtype == Gamepad)
    {
        if (m_controller.wii_data_format() == 1)
        {
            WiiIntermediateClassicDataFormat_t test;
            WiiClassicDataFormat1_t *data = (WiiClassicDataFormat1_t *)initialReport;
            test.rightStickX = 16;
            data->leftStickX = 32;
            data->leftStickY = 32;
            data->rightStickY = 16;
            data->rightStickX0 = test.rightStickX0;
            data->rightStickX21 = test.rightStickX21;
            data->rightStickX43 = test.rightStickX43;
            buttonsLowIdx = offsetof(WiiClassicDataFormat1_t, buttonsLow);
            buttonsHighIdx = offsetof(WiiClassicDataFormat1_t, buttonsHigh);
            m_size = sizeof(WiiClassicDataFormat1_t);
        }
        if (m_controller.wii_data_format() == 2)
        {
            WiiIntermediateClassicDataFormat2_t test;
            WiiClassicDataFormat2_t *data = (WiiClassicDataFormat2_t *)initialReport;
            test.leftStickX = 512;
            test.leftStickY = 512;
            test.rightStickX = 512;
            test.rightStickY = 512;
            data->leftStickX10 = test.leftStickX10;
            data->leftStickX92 = test.leftStickX92;
            data->leftStickY10 = test.leftStickY10;
            data->leftStickY92 = test.leftStickY92;
            data->rightStickX10 = test.rightStickX10;
            data->rightStickX92 = test.rightStickX92;
            data->rightStickY10 = test.rightStickY10;
            data->rightStickY92 = test.rightStickY92;
            buttonsLowIdx = offsetof(WiiClassicDataFormat2_t, buttonsLow);
            buttonsHighIdx = offsetof(WiiClassicDataFormat2_t, buttonsHigh);
            m_size = sizeof(WiiClassicDataFormat2_t);
        }
        if (m_controller.wii_data_format() == 3)
        {
            WiiClassicDataFormat3_t *data = (WiiClassicDataFormat3_t *)initialReport;
            data->leftStickX = 128;
            data->leftStickY = 128;
            data->rightStickX = 128;
            data->rightStickY = 128;
            buttonsLowIdx = offsetof(WiiClassicDataFormat3_t, buttonsLow);
            buttonsHighIdx = offsetof(WiiClassicDataFormat3_t, buttonsHigh);
            m_size = sizeof(WiiClassicDataFormat3_t);
        }
    }
    if (subtype == GuitarHeroGuitar)
    {
        WiiGuitarDataFormat3_t *data = (WiiGuitarDataFormat3_t *)m_buffer;
        data->leftStickX = 32;
        data->leftStickY = 32;
        buttonsLowIdx = offsetof(WiiGuitarDataFormat3_t, buttonsLow);
        buttonsHighIdx = offsetof(WiiGuitarDataFormat3_t, buttonsHigh);
        m_size = sizeof(WiiGuitarDataFormat3_t);
    }
    if (subtype == GuitarHeroDrums)
    {
        WiiDrumDataFormat3_t *data = (WiiDrumDataFormat3_t *)m_buffer;
        data->leftStickX = 32;
        data->leftStickY = 32;
        buttonsLowIdx = offsetof(WiiDrumDataFormat3_t, buttonsLow);
        buttonsHighIdx = offsetof(WiiDrumDataFormat3_t, buttonsHigh);
        m_size = sizeof(WiiDrumDataFormat3_t);
    }
    if (subtype == DjHeroTurntable)
    {
        WiiTurntableDataFormat3_t *data = (WiiTurntableDataFormat3_t *)m_buffer;
        data->leftStickX = 32;
        data->leftStickY = 32;
        buttonsLowIdx = offsetof(WiiTurntableDataFormat3_t, buttonsLow);
        buttonsHighIdx = offsetof(WiiTurntableDataFormat3_t, buttonsHigh);
        m_size = sizeof(WiiTurntableDataFormat3_t);
    }
}
void WiiEmulationDeviceInstance::process(bool full_poll, bool send_events)
{
    // TODO: do we need to limit poll rate with this
    // m_device->ready();
    // if the format changed, reinitialize
    if (lastFormat != m_controller.wii_data_format()) {
        lastFormat = m_controller.wii_data_format();
        initialize();
    }
    memcpy(m_buffer, initialReport, sizeof(initialReport));
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            mapping->update_wii(m_controller.wii_data_format(), m_buffer);
        }
        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
    }
    m_buffer[buttonsLowIdx] = ~m_buffer[buttonsLowIdx];
    m_buffer[buttonsHighIdx] = ~m_buffer[buttonsHighIdx];
    // TODO: euphoria led
    m_controller.setInputs(m_buffer, m_size);
}