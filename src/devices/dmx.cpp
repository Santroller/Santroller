#include "devices/dmx.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "config/config.hpp"
DMXDevice::DMXDevice(proto_DMXDevice device, uint16_t id) : LedDevice(id, device.channelCount, false, true), m_device(device)
{
}
void DMXDevice::begin()
{
    led_state = new uint32_t[m_device.channelCount];
    prev_led_state = new uint32_t[m_device.channelCount];
    memset(led_state, 0, sizeof(uint32_t) * m_device.channelCount);
    memset(prev_led_state, 0, sizeof(uint32_t) * m_device.channelCount);
    m_dmx.begin(m_device.pin);
}
void DMXDevice::end(bool full)
{
    m_dmx.end();
    delete[] led_state;
    delete[] prev_led_state;
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