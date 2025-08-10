#include "devices/crazyneck.hpp"
CrazyGuitarNeckDevice::CrazyGuitarNeckDevice(proto_CrazyGuitarNeckDevice device, uint16_t id) : Device(id), m_crazy_guitar_neck(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void CrazyGuitarNeckDevice::update() {
    m_crazy_guitar_neck.tick();
}