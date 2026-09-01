#include "devices/wii.hpp"
#include "managers/device_manager.hpp"

#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
WiiDevice::WiiDevice(const DeviceReloadState* state, proto_WiiDevice device, uint16_t id) : MidiDevice(state, id, false), m_extension(this, device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock), m_device(device)
{
    if (state)
    {
        m_lastExtType = state->wii_extension;
        m_lastValue = state->last_value;
    }
}

void WiiDevice::save_reload_state(DeviceReloadState& state) const
{
    MidiDevice::save_reload_state(state);
    state.wii_extension = m_lastExtType;
    state.last_value = m_lastValue;
}

WiiDevice::~WiiDevice() {}
void WiiDevice::begin()
{
    m_extension.begin();
}
void WiiDevice::end(bool full)
{
    m_extension.end();
}
void WiiDevice::rescan(bool first)
{
    MidiDevice::rescan(first);
    if (first)
    {
        DeviceManager::instance().add_assignable_device(DeviceManager::instance().get_root_device(m_id));
    }
}

void WiiDevice::update(bool full_poll, bool send_events)
{
    m_extension.tick();
    if (m_extension.mType != m_lastExtType || full_poll)
    {
        bool changed = m_extension.mType != m_lastExtType;
        printf("found wii ext: %d %d\r\n", m_lastExtType, m_extension.mType);
        m_lastExtType = m_extension.mType;
        proto_Event event = {which_event : proto_Event_wii_tag, event : {wii : {m_id, m_lastExtType}}};
        HIDConfigDevice::send_event(event, true);
        if (changed)
        {
            reload();
        }
    }
    MidiDevice::update(full_poll, send_events);
}
uint16_t WiiDevice::readAxis(proto_WiiAxisType type)
{
    return m_extension.readAxis(type);
}
bool WiiDevice::readButton(proto_WiiButtonType type)
{
    return m_extension.readButton(type);
}
bool WiiDevice::is_wii_extension(WiiExtType type)
{
    return m_extension.mType == type;
}

bool WiiDevice::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.scl || pin == m_device.i2c.sda;
}