#include "devices/accelerometer.hpp"
#include "config.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
AccelerometerDevice::AccelerometerDevice(proto_AccelerometerDevice device, uint16_t id) : Device(id), m_accelerometer(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock), m_device(device)
{
}

void AccelerometerDevice::update(bool full_poll, bool send_events)
{
    m_accelerometer.tick();
    if (send_events && (m_lastConnected != m_accelerometer.is_connected() || full_poll))
    {
        m_lastConnected = m_accelerometer.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event);
    }
}

bool AccelerometerDevice::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.scl || pin == m_device.i2c.sda;
}