#include "input/wii.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

WiiAxisInput::WiiAxisInput(proto_WiiAxisInput input, std::shared_ptr<WiiDevice> device) : m_input(input), m_device(device)
{
}
bool WiiAxisInput::tickDigital()
{
    return m_device->readAxis(m_input.axis) > 0;
}
uint16_t WiiAxisInput::tickAnalog()
{
    return m_device->readAxis(m_input.axis);
}
void WiiAxisInput::setup()
{
}
WiiButtonInput::WiiButtonInput(proto_WiiButtonInput input, std::shared_ptr<WiiDevice> device) : m_input(input), m_device(device)
{
}
bool WiiButtonInput::tickDigital()
{
    return m_device->readButton(m_input.button) > 0;
}
uint16_t WiiButtonInput::tickAnalog()
{
    return m_device->readButton(m_input.button) ? 65535 : 0;
}
void WiiButtonInput::setup()
{
}
WiiExtensionTypeInput::WiiExtensionTypeInput(proto_WiiExtensionTypeInput input, std::shared_ptr<WiiDevice> device) : m_input(input), m_device(device)
{
}
bool WiiExtensionTypeInput::tickDigital()
{
    return m_device->isExtension(m_input.ext);
}
uint16_t WiiExtensionTypeInput::tickAnalog()
{
    return m_device->isExtension(m_input.ext) ? 65535 : 0;
}
void WiiExtensionTypeInput::setup()
{
}
WiiMidiNoteInput::WiiMidiNoteInput(proto_MidiNoteInput input, std::shared_ptr<WiiDevice> device) : m_input(input), m_device(device)
{
}
bool WiiMidiNoteInput::tickDigital()
{
    return m_device->readMidiNote(m_input.note) > 0;
}
uint16_t WiiMidiNoteInput::tickAnalog()
{
    return m_device->readMidiNote(m_input.note);
}
void WiiMidiNoteInput::setup()
{
}
WiiMidiControlChangeInput::WiiMidiControlChangeInput(proto_MidiControlChangeInput input, std::shared_ptr<WiiDevice> device) : m_input(input), m_device(device)
{
}
bool WiiMidiControlChangeInput::tickDigital()
{
    return m_device->readMidiControlChange(m_input.cc) > 0;
}
uint16_t WiiMidiControlChangeInput::tickAnalog()
{
    return m_device->readMidiControlChange(m_input.cc);
}
void WiiMidiControlChangeInput::setup()
{
}
WiiMidiPitchBendInput::WiiMidiPitchBendInput(proto_MidiPitchBendInput input, std::shared_ptr<WiiDevice> device) : m_input(input), m_device(device)
{
}
bool WiiMidiPitchBendInput::tickDigital()
{
    return m_device->readMidiPitchBend() != 0;
}
uint16_t WiiMidiPitchBendInput::tickAnalog()
{
    return m_device->readMidiPitchBend();
}
void WiiMidiPitchBendInput::setup()
{
}