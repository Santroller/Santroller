#include "devices/apa102.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "config/config.hpp"
APA102Device::APA102Device(proto_APA102Device device, uint16_t id) : LedDevice(id, device.count, true, true), m_apa102(device.spi.block, device.spi.mosi, device.spi.sck, device.count, device.type), m_device(device)
{
}

void APA102Device::begin()
{
    m_apa102.begin();
    led_state = new uint32_t[m_device.count];
    prev_led_state = new uint32_t[m_device.count];
    memset(led_state, 0, sizeof(uint32_t) * m_device.count);
    memset(prev_led_state, 0, sizeof(uint32_t) * m_device.count);
}
void APA102Device::end(bool full)
{
    m_apa102.end();
    delete[] led_state;
    delete[] prev_led_state;
}
void APA102Device::update(bool full_poll, bool send_events)
{
    m_apa102.putLeds(led_state, m_device.count);
}

bool APA102Device::using_pin(uint8_t pin)
{
    return pin == m_device.spi.mosi || pin == m_device.spi.sck;
}