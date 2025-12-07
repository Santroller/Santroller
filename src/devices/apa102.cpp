#include "devices/apa102.hpp"
#include "events.pb.h"
#include "main.hpp"
APA102Device::APA102Device(proto_APA102Device device, uint16_t id) : LedDevice(id, true), m_apa102(device.spi.block, device.spi.mosi, device.spi.sck, device.count, device.type)
{
}

void APA102Device::update(bool full_poll)
{
    m_apa102.begin();
    for (int i = 0; i < m_device.count; i++)
    {
        uint8_t r = led_state[i] & 0xff;
        uint8_t g = (led_state[i] >> 8) & 0xff;
        uint8_t b = (led_state[i] >> 16) & 0xff;
        uint8_t brightness = (led_state[i] >> 24) & 0xff;
        m_apa102.putLed(brightness, r, g, b);
    }
    m_apa102.end();
}