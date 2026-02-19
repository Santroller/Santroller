#include "devices/wii.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
WiiDevice::WiiDevice(proto_WiiDevice device, uint16_t id) : MidiDevice(id), m_extension(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock), m_device(device)
{
}
void WiiDevice::rescan(bool first)
{
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
    if (m_extension.mType != m_lastExtType || full_poll || resend)
    {
        m_lastExtType = m_extension.mType;
        if (send_events)
        {
            proto_Event event = {which_event : proto_Event_wii_tag, event : {wii : {m_id, m_lastExtType}}};
            resend = !HIDConfigDevice::send_event(event);
        }
        rescan(false);
    }
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
uint16_t WiiDevice::readMidiNote(uint8_t note)
{
    return m_extension.midiInterface.midiVelocities[note] << 8;
}
uint16_t WiiDevice::readMidiControlChange(uint8_t cc)
{
    switch (cc)
    {
    case MIDI_CONTROL_COMMAND_MOD_WHEEL:
        return m_extension.midiInterface.midiModWheel << 8;
    case MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL:
        return m_extension.midiInterface.midiSustainPedal << 8;
    default:
        return 0;
    }
    return 0;
}
int16_t WiiDevice::readMidiPitchBend()
{
    return m_extension.midiInterface.midiPitchWheel;
}

bool WiiDevice::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.scl || pin == m_device.i2c.sda;
}