#include "devices/mpr121.hpp"
MPR121Device::MPR121Device(proto_Mpr121Device device, uint16_t id) : Device(id), m_mpr121(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void MPR121Device::update() {
    m_mpr121.tick();
}