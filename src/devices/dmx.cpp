#include "devices/dmx.hpp"
#include "pio_pins.hpp"
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
    m_started = false;
    if (m_device.pin >= 0 && m_device.pin < (int)NUM_BANK0_GPIOS)
    {
        // Any PIO that can reach the pin (on an RP2350B, a PIO reaches GP0 - GP31 or GP16 - GP47)
        for (uint i = 0; i < NUM_PIOS && !m_started; i++)
        {
            PIO pio = pio_get_instance(i);
            m_started = pio_try_reach_pins(pio, {(uint)m_device.pin}) && m_dmx.begin(m_device.pin, pio) == DmxOutput::SUCCESS;
        }
    }
}
void DMXDevice::end(bool full)
{
    if (m_started)
    {
        m_dmx.end();
        m_started = false;
    }
    delete[] led_state;
    delete[] prev_led_state;
}

void DMXDevice::update(bool full_poll, bool send_events)
{
    if (!m_started || m_dmx.busy())
    {
        return;
    }
    m_dmx.write((uint8_t *)led_state, m_device.channelCount);
    // m_dmx.putLeds((uint8_t *)led_state);
}

bool DMXDevice::using_pin(uint8_t pin)
{
    return pin == m_device.pin;
}