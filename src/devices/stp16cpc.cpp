#include "devices/stp16cpc.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "config.hpp"
STP16CPCDevice::STP16CPCDevice(proto_STP16CPCDevice device, uint16_t id) : LedDevice(id, false), m_stp15cpc(device.spi.block, device.spi.mosi, device.spi.sck, device.oe, device.le, device.count), m_device(device)
{
}

void STP16CPCDevice::update(bool full_poll)
{
    m_stp15cpc.putLeds((uint8_t *)led_state);
}

bool STP16CPCDevice::using_pin(uint8_t pin)
{
    return pin == m_device.spi.mosi || pin == m_device.spi.miso || pin == m_device.spi.sck  || pin == m_device.le || pin == m_device.oe;
}