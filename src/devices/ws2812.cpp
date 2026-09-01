#include "devices/ws2812.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "config/config.hpp"
WS2812Device::WS2812Device(proto_WS2812Device device, uint16_t id) : LedDevice(id, device.count, true, false), m_ws2812(device.pin, device.count, device.type), m_device(device)
{
}

void WS2812Device::begin()
{
    led_state = new uint32_t[m_device.count];
    prev_led_state = new uint32_t[m_device.count];
    memset(led_state, 0, sizeof(uint32_t) * m_device.count);
    memset(prev_led_state, 0, sizeof(uint32_t) * m_device.count);
    m_ws2812.begin();
}

void WS2812Device::end(bool full)
{
    m_ws2812.end();
    delete[] led_state;
    delete[] prev_led_state;
    led_state = nullptr;
    prev_led_state = nullptr;
}

void WS2812Device::update(bool full_poll, bool send_events)
{
    if (memcmp(prev_led_state, led_state, sizeof(uint32_t) * m_device.count) != 0)
    {
        m_ws2812.putLeds(led_state, m_device.count);
        memcpy(prev_led_state, led_state, sizeof(uint32_t) * m_device.count);
    }
}

bool WS2812Device::using_pin(uint8_t pin)
{
    return pin == m_device.pin;
}