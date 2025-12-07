#include "devices/ws2812.hpp"
#include "events.pb.h"
#include "main.hpp"
WS2812Device::WS2812Device(proto_WS2812Device device, uint16_t id) : LedDevice(id, true), m_device(device), m_ws2812(device.pin, device.type)
{
}

void WS2812Device::update(bool full_poll)
{
    for (int i = 0; i < m_device.count; i++)
    {
        uint8_t r = led_state[i] & 0xff;
        uint8_t g = (led_state[i] >> 8) & 0xff;
        uint8_t b = (led_state[i] >> 16) & 0xff;
        m_ws2812.putLed(r, g, b);
    }
}