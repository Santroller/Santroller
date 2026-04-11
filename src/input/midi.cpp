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

MidiProGuitarButtonInput::MidiProGuitarButtonInput(proto_MidiProGuitarButtonInput input, std::shared_ptr<ProGuitarMidiDevice> device) : m_input(input), m_device(device)
{
}
bool MidiProGuitarButtonInput::tickDigital()
{
    return m_device->readProGuitarButton(m_input.button) ? 65535 : 0;
}
uint16_t MidiProGuitarButtonInput::tickAnalog()
{
    return m_device->readProGuitarButton(m_input.button);
}
void MidiProGuitarButtonInput::setup()
{
}

MidiProGuitarAxisInput::MidiProGuitarAxisInput(proto_MidiProGuitarAxisInput input, std::shared_ptr<ProGuitarMidiDevice> device) : m_input(input), m_device(device)
{
}
bool MidiProGuitarAxisInput::tickDigital()
{
    return m_device->readProGuitarAxis(m_input.axis) > 0;
}
uint16_t MidiProGuitarAxisInput::tickAnalog()
{
    return m_device->readProGuitarAxis(m_input.axis);
}
void MidiProGuitarAxisInput::setup()
{
}