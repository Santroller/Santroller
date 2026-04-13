#include "devices/ads1115.hpp"
#include "events.pb.h"
#include "config.hpp"
#include "main.hpp"
#include "usb/device/hid_device.h"
ADS1115Device::ADS1115Device(proto_ADS1115Device device, uint16_t id) : Device(id), ads1115(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock, device.interrupt), m_device(device)
{
}

void ADS1115Device::update(bool full_poll, bool send_events)
{
    ads1115.tick();
    if (m_lastConnected != ads1115.is_connected() || full_poll)
    {
        m_lastConnected = ads1115.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool ADS1115Device::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.scl || pin == m_device.i2c.sda || pin == m_device.interrupt;
}