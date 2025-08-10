#include "devices/wii.hpp"
WiiDevice::WiiDevice(proto_WiiDevice device, uint16_t id) : Device(id), m_extension(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void WiiDevice::update() {
    m_extension.tick();
    // if we have just picked up a device we should be emitting an event to the tool stating as such
}