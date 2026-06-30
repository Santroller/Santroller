#include "devices/dmx.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "config.hpp"
DMXDevice::DMXDevice(proto_DMXDevice device, uint16_t id) : LedDevice(id, false, true), m_device(device)
{
}
void DMXDevice::begin()
{
    m_dmx.begin(m_device.pin);
}
void DMXDevice::end(bool full)
{
    m_dmx.end();
}

void DMXDevice::update(bool full_poll, bool send_events)
{
    if (m_dmx.busy()) {
        return;
    }
    m_dmx.write((uint8_t*)led_state, m_device.channelCount);
    // m_dmx.putLeds((uint8_t *)led_state);
}

bool DMXDevice::using_pin(uint8_t pin)
{
    return pin == m_device.pin;
}