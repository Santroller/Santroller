#include "devices/ps2.hpp"
#include "managers/device_manager.hpp"

#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "utils.h"
#include "stdio.h"
#include <algorithm>
PS2Device::PS2Device(const DeviceReloadState* state, proto_PSXDevice device, uint16_t id) : Device(id), m_controller(device.spi.block, device.spi.sck, device.spi.mosi, device.spi.miso, device.spi.clock, device.attPin, device.ackPin), m_device(device)
{
    if (state)
    {
        m_last_value = state->last_value;
        m_lastControllerType = state->ps2_controller;
    }
}

void PS2Device::save_reload_state(DeviceReloadState& state) const
{
    state.valid = true;
    state.last_value = m_last_value;
    state.ps2_controller = m_lastControllerType;
}
void PS2Device::begin()
{
    m_controller.begin();
}
void PS2Device::end(bool full)
{
    m_controller.end();
}
void PS2Device::rescan(bool first)
{
    if (first)
    {
        DeviceManager::instance().add_assignable_device(DeviceManager::instance().get_root_device(m_id));
    }
}
void PS2Device::update(bool full_poll, bool send_events)
{
    m_controller.tick();
    if (m_controller.type != m_lastControllerType || full_poll)
    {
        bool changed = m_controller.type != m_lastControllerType;
        m_lastControllerType = m_controller.type;
        proto_Event event = {which_event : proto_Event_ps2_tag, event : {ps2 : {m_id, m_lastControllerType}}};
        HIDConfigDevice::send_event(event, true);
        if (changed)
        {
            reload();
        }
    }
}

bool PS2Device::using_pin(uint8_t pin)
{
    return pin == m_device.spi.mosi || pin == m_device.spi.miso || pin == m_device.spi.sck || pin == m_device.ackPin || pin == m_device.attPin;
}
uint16_t PS2Device::read_axis(proto_PS2AxisType type)
{
    return m_controller.read_axis(type);
}
bool PS2Device::read_button(proto_PS2ButtonType type)
{
    return m_controller.read_button(type);
}
bool PS2Device::is_ps2_device(PS2ControllerType type)
{
    return m_lastControllerType == type;
}