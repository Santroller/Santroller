#include "devices/wii.hpp"
#include "events.pb.h"
#include "main.hpp"
WiiDevice::WiiDevice(proto_WiiDevice device, uint16_t id) : Device(id), m_extension(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void WiiDevice::update(bool full_poll)
{
    m_extension.tick();
    if (m_extension.mType != m_lastExtType || full_poll)
    {

        m_lastExtType = m_extension.mType;
        proto_Event event = {which_event : proto_Event_wii_tag, event : {wii : {m_id, m_lastExtType}}};
        send_event(event);
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