#include "input/midi.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"


MidiNoteInput::MidiNoteInput(proto_MidiNoteInput input, std::shared_ptr<MidiDeviceWithChannel> device) : m_input(input), m_device(device)
{
}
bool MidiNoteInput::tickDigital()
{
    return m_device->readMidiNote(m_input.note) > 0;
}
uint16_t MidiNoteInput::tickAnalog()
{
    return m_device->readMidiNote(m_input.note);
}
void MidiNoteInput::setup()
{
}
MidiControlChangeInput::MidiControlChangeInput(proto_MidiControlChangeInput input, std::shared_ptr<MidiDeviceWithChannel> device) : m_input(input), m_device(device)
{
}
bool MidiControlChangeInput::tickDigital()
{
    return m_device->readMidiControlChange(m_input.cc) > 0;
}
uint16_t MidiControlChangeInput::tickAnalog()
{
    return m_device->readMidiControlChange(m_input.cc);
}
void MidiControlChangeInput::setup()
{
}
MidiPitchBendInput::MidiPitchBendInput(proto_MidiPitchBendInput input, std::shared_ptr<MidiDeviceWithChannel> device) : m_input(input), m_device(device)
{
}
bool MidiPitchBendInput::tickDigital()
{
    return m_device->readMidiPitchBend() != 0;
}
uint16_t MidiPitchBendInput::tickAnalog()
{
    return m_device->readMidiPitchBend();
}
void MidiPitchBendInput::setup()
{
}