#include "devices/accelerometer.hpp"
AccelerometerDevice::AccelerometerDevice(proto_AccelerometerDevice device, uint16_t id) : Device(id), m_accelerometer(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void AccelerometerDevice::update(bool resend_events) {
    m_accelerometer.tick();
}