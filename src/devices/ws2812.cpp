#include "devices/ws2812.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "stdio.h"
WS2812Device::WS2812Device(proto_WS2812Device device, uint16_t id) : LedDevice(id), m_device(device), m_ws2812(device.pin, device.type >= Ws2812Rgbw)
{
    printf("ws2812: %d %d %d\r\n", device.pin, device.count, device.type);
    memset(led_state, 0, sizeof(led_state));
}

void WS2812Device::update(bool full_poll)
{
    for (int i = 0; i < m_device.count; i++)
    {
        uint8_t r = led_state[i] & 0xff;
        uint8_t g = (led_state[i] >> 8) & 0xff;
        uint8_t b = (led_state[i] >> 16) & 0xff;
        switch (m_device.type)
        {
        case Ws2812Rgbw:
        case Ws2812Rgb:
            m_ws2812.putLed(r, g, b);
            break;
        case Ws2812Rbgw:
        case Ws2812Rbg:
            m_ws2812.putLed(r, b, g);
            break;
        case Ws2812Grbw:
        case Ws2812Grb:
            m_ws2812.putLed(g, r, b);
            break;
        case Ws2812Gbrw:
        case Ws2812Gbr:
            m_ws2812.putLed(g, b, r);
            break;
        case Ws2812Brgw:
        case Ws2812Brg:
            m_ws2812.putLed(b, r, g);
            break;
        case Ws2812Bgrw:
        case Ws2812Bgr:
            m_ws2812.putLed(b, g, r);
            break;
        }
    }
}