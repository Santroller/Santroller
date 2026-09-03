#include "devices/wii_emulation.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "utils.h"
#include "stdio.h"
#include <algorithm>
WiiExtensionEmulationDevice::WiiExtensionEmulationDevice(proto_WiiEmulationDevice device, uint16_t id) : Device(id), m_device(device)
{
}

void WiiExtensionEmulationDevice::begin()
{
}
void WiiExtensionEmulationDevice::end(bool full)
{
}
void WiiExtensionEmulationDevice::rescan(bool first)
{
}
void WiiExtensionEmulationDevice::update(bool full_poll, bool send_events)
{
}

bool WiiExtensionEmulationDevice::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.sda || pin == m_device.i2c.scl;
}
uint16_t WiiExtensionEmulationDevice::read_axis(proto_PS2AxisType type)
{
    return 0;
}
bool WiiExtensionEmulationDevice::read_button(proto_PS2ButtonType type)
{
    return false;
}