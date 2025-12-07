#include "devices/apa102.hpp"
#include "events.pb.h"
#include "main.hpp"
void LedDevice::set_led(uint8_t i, uint8_t r, uint8_t g, uint8_t b)
{
    if (m_supportsColour)
    {
        led_state[i] = (r) | (g << 8) | (b << 16);
    }
    else if (r || g || b)
    {
        led_state[i / 32] = 1 << (i % 32);
    }
    else
    {
        led_state[i / 32] &= ~(1 << (i % 32));
    }
}

LedDevice::LedDevice(uint16_t id, bool supportsColour) : Device(id), m_supportsColour(supportsColour)
{
    memset(led_state, 0, sizeof(led_state));
}