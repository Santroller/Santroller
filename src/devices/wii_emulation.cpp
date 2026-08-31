#include "devices/wii_emulation.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "utils.h"
#include "stdio.h"
#include <algorithm>
WiiEmulationDevice::WiiEmulationDevice(proto_WiiEmulationDevice device, uint16_t id) : Device(id), m_device(device)
{
}

void WiiEmulationDevice::begin()
{
}
void WiiEmulationDevice::end(bool full)
{
}
void WiiEmulationDevice::rescan(bool first)
{
}
void WiiEmulationDevice::update(bool full_poll, bool send_events)
{
}

bool WiiEmulationDevice::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.sda || pin == m_device.i2c.scl;
}
uint16_t WiiEmulationDevice::readAxis(proto_PS2AxisType type)
{
    return 0;
}
bool WiiEmulationDevice::readButton(proto_PS2ButtonType type)
{
    return false;
}