#include "devices/infinium_fader.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"

InfiniumFaderDevice::InfiniumFaderDevice(proto_InfiniumFaderDevice device, uint16_t id)
    : Device(id), fader(device.uart.block, device.uart.tx, device.uart.rx, device.uart.baudrate), m_device(device)
{
}

void InfiniumFaderDevice::begin()
{
    fader.begin();
}

void InfiniumFaderDevice::end(bool full)
{
    fader.end();
}

void InfiniumFaderDevice::update(bool full_poll, bool send_events)
{
    fader.tick();
    if (m_lastConnected != fader.is_connected() || full_poll)
    {
        m_lastConnected = fader.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool InfiniumFaderDevice::using_pin(uint8_t pin)
{
    return pin == m_device.uart.rx || pin == m_device.uart.tx;
}
