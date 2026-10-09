#include "devices/snes.hpp"
#include "events.pb.h"
#include "emulation/usb/hid_device.h"
#include <cstring>
#include <stdio.h>

// Without a free PIO state machine, reading the pad bit-bangs about 200us of clock pulses,
// so don't do that every loop
#define SNES_BITBANG_POLL_INTERVAL_US 2000

SNESDevice::SNESDevice(proto_SNESDevice device, uint16_t id)
    : Device(id), m_device(device), m_pad(device.clockPin, device.latchPin, device.dataPin)
{
}

bool SNESDevice::matches_reload_config(const proto_Device &config) const
{
    return config.which_device == proto_Device_snes_tag &&
           memcmp(&m_device, &config.device.snes, sizeof(m_device)) == 0;
}

void SNESDevice::begin()
{
    m_pad.begin();
    if (!m_pad.using_pio())
    {
        printf("SNES: no free PIO state machine, bit-banging the pad instead\r\n");
    }
}

void SNESDevice::end(bool full)
{
    m_pad.end();
}

SNESControllerType SNESDevice::controller_type() const
{
    switch (m_pad.type)
    {
    case SNES_PAD_BASIC:
        return SNESControllerSNES;
    case SNES_PAD_NES:
        return SNESControllerNES;
    case SNES_PAD_MOUSE:
        return SNESControllerMouse;
    default:
        return SNESControllerNone;
    }
}

void SNESDevice::update(bool full_poll, bool send_events)
{
    uint32_t now = time_us_32();
    if (m_pad.using_pio() || now - m_last_poll_us >= SNES_BITBANG_POLL_INTERVAL_US)
    {
        m_last_poll_us = now;
        m_pad.poll();
    }
    auto type = controller_type();
    if (type != m_last_type || full_poll)
    {
        m_last_type = type;
        proto_Event event = {which_event : proto_Event_snes_tag, event : {snes : {m_id, type}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool SNESDevice::using_pin(uint8_t pin)
{
    return pin == m_device.clockPin || pin == m_device.latchPin || pin == m_device.dataPin;
}

uint16_t SNESDevice::read_axis(proto_SNESAxisType type)
{
    if (m_pad.type != SNES_PAD_MOUSE)
    {
        return 0;
    }
    switch (type)
    {
    case SNESAxisMouseX:
        return m_pad.mouseX << 8;
    case SNESAxisMouseY:
        return m_pad.mouseY << 8;
    default:
        return 0;
    }
}

bool SNESDevice::read_button(proto_SNESButtonType type)
{
    if (m_pad.type == SNES_PAD_MOUSE)
    {
        // SNESpad puts the mouse's left button in buttonB and the right one in buttonA
        switch (type)
        {
        case SNESButtonMouseLeft:
            return m_pad.buttonB;
        case SNESButtonMouseRight:
            return m_pad.buttonA;
        default:
            return false;
        }
    }
    switch (type)
    {
    case SNESButtonB:
        return m_pad.buttonB;
    case SNESButtonY:
        return m_pad.buttonY;
    case SNESButtonSelect:
        return m_pad.buttonSelect;
    case SNESButtonStart:
        return m_pad.buttonStart;
    case SNESButtonDpadUp:
        return m_pad.directionUp;
    case SNESButtonDpadDown:
        return m_pad.directionDown;
    case SNESButtonDpadLeft:
        return m_pad.directionLeft;
    case SNESButtonDpadRight:
        return m_pad.directionRight;
    case SNESButtonA:
        return m_pad.buttonA;
    case SNESButtonX:
        return m_pad.buttonX;
    case SNESButtonL:
        return m_pad.buttonL;
    case SNESButtonR:
        return m_pad.buttonR;
    default:
        return false;
    }
}
