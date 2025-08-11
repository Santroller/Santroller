#include "devices/max1704x.hpp"
Max1704XDevice::Max1704XDevice(proto_Max1704xDevice device, uint16_t id) : Device(id), m_max1704x(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void Max1704XDevice::update(bool resend_events) {
    m_max1704x.tick();
}