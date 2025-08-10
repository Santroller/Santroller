#include "devices/gh5neck.hpp"
GH5NeckDevice::GH5NeckDevice(proto_GuitarHero5NeckDevice device, uint16_t id) : Device(id), m_gh5_neck(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void GH5NeckDevice::update() {
    m_gh5_neck.tick();
}