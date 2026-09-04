#include "input/midi.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"


MidiNoteInput::MidiNoteInput(proto_MidiNoteInput input, std::shared_ptr<MidiDevice> device) : m_input(input), m_device(device)
{
    printf("MidiNoteInput: %d\r\n", m_input.note);
}
bool MidiNoteInput::tick_digital()
{
    uint16_t value;
    return consume_event(value);
}
uint16_t MidiNoteInput::tick_analog()
{
    uint16_t value;
    consume_event(value);
    return value;
}
bool MidiNoteInput::consume_event(uint16_t &value)
{
    return m_device->consume_midi_note_event(m_input.channel - 1, m_input.note, m_last_event_sequence, value);
}
void MidiNoteInput::setup()
{
}
MidiControlChangeInput::MidiControlChangeInput(proto_MidiControlChangeInput input, std::shared_ptr<MidiDevice> device) : m_input(input), m_device(device)
{
}
bool MidiControlChangeInput::tick_digital()
{
    return m_device->read_midi_control_change(m_input.channel - 1, m_input.cc) > 0;
}
uint16_t MidiControlChangeInput::tick_analog()
{
    return m_device->read_midi_control_change(m_input.channel - 1, m_input.cc);
}
void MidiControlChangeInput::setup()
{
}
MidiPitchBendInput::MidiPitchBendInput(proto_MidiPitchBendInput input, std::shared_ptr<MidiDevice> device) : m_input(input), m_device(device)
{
}
bool MidiPitchBendInput::tick_digital()
{
    return m_device->read_midi_pitch_bend(m_input.channel - 1) != 0;
}
uint16_t MidiPitchBendInput::tick_analog()
{
    return m_device->read_midi_pitch_bend(m_input.channel - 1);
}
void MidiPitchBendInput::setup()
{
}

MidiProGuitarButtonInput::MidiProGuitarButtonInput(proto_MidiProGuitarButtonInput input, std::shared_ptr<ProGuitarMidiDevice> device) : m_input(input), m_device(device)
{
}
bool MidiProGuitarButtonInput::tick_digital()
{
    return m_device->read_pro_guitar_button(m_input.button);
}
uint16_t MidiProGuitarButtonInput::tick_analog()
{
    return m_device->read_pro_guitar_button(m_input.button) ? 65535 : 0;
}
void MidiProGuitarButtonInput::setup()
{
}

MidiProGuitarAxisInput::MidiProGuitarAxisInput(proto_MidiProGuitarAxisInput input, std::shared_ptr<ProGuitarMidiDevice> device) : m_input(input), m_device(device)
{
}
bool MidiProGuitarAxisInput::tick_digital()
{
    return m_device->read_pro_guitar_axis(m_input.axis) > 0;
}
uint16_t MidiProGuitarAxisInput::tick_analog()
{
    return m_device->read_pro_guitar_axis(m_input.axis);
}
void MidiProGuitarAxisInput::setup()
{
}