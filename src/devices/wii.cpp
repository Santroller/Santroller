#include "devices/wii.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
WiiDevice::WiiDevice(proto_WiiDevice device, uint16_t id) : MidiDevice(id, false), m_extension(this, device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock), m_device(device)
{
}
void WiiDevice::rescan(bool first)
{
    MidiDevice::rescan(first);
    if (first)
    {
        assignable_devices.push_back(active_devices.back());
        update(false, false);
        m_has_scanned = true;
    }
    else if (m_has_scanned && HIDConfigDevice::tool_closed())
    {
        reload();
    }
}
void WiiDevice::update(bool full_poll, bool send_events)
{
    m_extension.tick();
    if (m_extension.mType != m_lastExtType || full_poll)
    {
        bool changed = m_extension.mType != m_lastExtType;
        m_lastExtType = m_extension.mType;
        proto_Event event = {which_event : proto_Event_wii_tag, event : {wii : {m_id, m_lastExtType}}};
        HIDConfigDevice::send_event(event, true);
        if (changed)
        {
            rescan(false);
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