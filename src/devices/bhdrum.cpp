#include "devices/bhdrum.hpp"
BandHeroDrumDevice::BandHeroDrumDevice(proto_BandHeroDrumDevice device, uint16_t id) : Device(id), m_band_hero_drum(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void BandHeroDrumDevice::update() {
    m_band_hero_drum.tick();
}