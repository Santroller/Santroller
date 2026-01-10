#include "devices/accelerometer.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
AccelerometerDevice::AccelerometerDevice(proto_AccelerometerDevice device, uint16_t id) : Device(id), m_accelerometer(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void AccelerometerDevice::update(bool full_poll) {
    m_accelerometer.tick();
    if (m_lastConnected != m_accelerometer.isConnected() || full_poll) {
        m_lastConnected = m_accelerometer.isConnected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event);
    }
}