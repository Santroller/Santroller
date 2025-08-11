#include "devices/djh.hpp"
DjHeroTurntableDevice::DjHeroTurntableDevice(proto_DJHeroTurntableDevice device, uint16_t id) : Device(id), m_turntable(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock, device.left)
{
}

void DjHeroTurntableDevice::update(bool resend_events) {
    m_turntable.tick();
}