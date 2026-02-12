#include "devices/ws2812.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "config.hpp"
WS2812Device::WS2812Device(proto_WS2812Device device, uint16_t id) : LedDevice(id, true), m_ws2812(device.pin, device.count, device.type), m_device(device)
{
}

void WS2812Device::update(bool full_poll)
{
    if (memcmp(prev_led_state, led_state, sizeof(led_state)) != 0)
    {
        m_ws2812.putLeds(led_state, m_device.count);
        memcpy(prev_led_state, led_state, sizeof(led_state));
    }
}

bool WS2812Device::using_pin(uint8_t pin)
{
    return pin == m_device.pin;
}