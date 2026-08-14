#include "devices/ps2_emulation.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
#include "utils.h"
#include "stdio.h"
#include <algorithm>
PSXEmulationDevice::PSXEmulationDevice(std::shared_ptr<PSXEmulationDevice> old, proto_PSXEmulationDevice device, uint16_t id) : Device(id), m_controller(device.clockPin, device.commandPin, device.dataPin, device.attentionPin, device.acknowledgePin), m_device(device)
{
    if (old)
    {
        m_lastValue = old->m_lastValue;
        m_lastControllerType = old->m_lastControllerType;
        m_controller.load_state(&old->m_controller);
    }
}

void PSXEmulationDevice::begin()
{
    m_controller.begin();
}
void PSXEmulationDevice::end(bool full)
{
    m_controller.end();
}
void PSXEmulationDevice::rescan(bool first)
{
    if (first)
    {
        assignable_devices.push_back(root_devices[m_id]);
    }
}
void PSXEmulationDevice::update(bool full_poll, bool send_events)
{
    m_controller.tick();
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
bool PSXEmulationDevice::is_ps2_device(PS2ControllerType type)
{
    return false;
}