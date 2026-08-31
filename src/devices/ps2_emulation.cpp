#include "devices/ps2_emulation.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "utils.h"
#include "stdio.h"
#include <algorithm>
PSXEmulationDevice::PSXEmulationDevice(proto_PSXEmulationDevice device, uint16_t id) : Device(id), m_device(device)
{
}

void PSXEmulationDevice::begin()
{
}
void PSXEmulationDevice::end(bool full)
{
}
void PSXEmulationDevice::rescan(bool first)
{
}
void PSXEmulationDevice::update(bool full_poll, bool send_events)
{
}

bool PSXEmulationDevice::using_pin(uint8_t pin)
{
    return pin == m_device.clockPin || pin == m_device.commandPin || pin == m_device.dataPin || pin == m_device.attentionPin || pin == m_device.acknowledgePin;
}
uint16_t PSXEmulationDevice::readAxis(proto_PS2AxisType type)
{
    return 0;
}
bool PSXEmulationDevice::readButton(proto_PS2ButtonType type)
{
    return false;
}