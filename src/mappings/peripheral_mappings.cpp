#include "class/hid/hid.h"
#include "config/config.hpp"
#include "events.pb.h"
#include "instance.hpp"
#include "main.hpp"
#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include <pb_encode.h>
#include <stdint.h>
#include <utils.h>

WheelButtonMapping::WheelButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void WheelButtonMapping::update_hid(uint8_t *buf)
{
    
}
void WheelButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    
}
void WheelButtonMapping::update_switch(uint8_t *buf)
{
    
}

void WheelButtonMapping::update_ps2(uint8_t *buf)
{
    
}

void WheelButtonMapping::update_ps3(uint8_t *buf)
{
    
}

void WheelButtonMapping::update_ps4(uint8_t *buf)
{
    
}

void WheelButtonMapping::update_ps5(uint8_t *buf)
{
    
}

void WheelButtonMapping::update_xinput(uint8_t *buf)
{
    
}
void WheelButtonMapping::update_ogxbox(uint8_t *buf)
{
    
}
void WheelButtonMapping::update_xboxone(uint8_t *buf)
{
}

WheelAxisMapping::WheelAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}

void WheelAxisMapping::update_hid(uint8_t *buf)
{
    
}
void WheelAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    
}
void WheelAxisMapping::update_switch(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_ps2(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_ps3(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_ps4(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_ps5(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_xinput(uint8_t *buf)
{
    
}
void WheelAxisMapping::update_ogxbox(uint8_t *buf)
{
    
}
void WheelAxisMapping::update_xboxone(uint8_t *buf)
{
}

KeyboardButtonMapping::KeyboardButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void KeyboardButtonMapping::update_hid(uint8_t *buf)
{
    if (!m_last_value) {
        return;
    }
    if (m_mapping.mapping.which_mapping == proto_Output_consumerKey_tag) {
        m_profile->consumer_state.set_key(m_mapping.mapping.mapping.consumerKey);
    } else {
        m_profile->keyboard_state.set_key(m_mapping.mapping.mapping.keycode);
    }
}
void KeyboardButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    
}
void KeyboardButtonMapping::update_switch(uint8_t *buf)
{
    
}

void KeyboardButtonMapping::update_ps2(uint8_t *buf)
{
    
}

void KeyboardButtonMapping::update_ps3(uint8_t *buf)
{
    
}

void KeyboardButtonMapping::update_ps4(uint8_t *buf)
{
    
}

void KeyboardButtonMapping::update_ps5(uint8_t *buf)
{
    
}

void KeyboardButtonMapping::update_xinput(uint8_t *buf)
{
    
}
void KeyboardButtonMapping::update_ogxbox(uint8_t *buf)
{
    
}
void KeyboardButtonMapping::update_xboxone(uint8_t *buf)
{
}

MouseAxisMapping::MouseAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}

void MouseAxisMapping::update_hid(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    auto &axes = m_profile->mouse_state.axes;
    int32_t deflection = static_cast<int32_t>(m_calibrated_value) - 32768;
    switch (m_mapping.mapping.mapping.mouseAxis)
    {
    case Mouse_MoveX:
        axes[MouseState::X] = deflection;
        break;
    case Mouse_MoveY:
        axes[MouseState::Y] = deflection;
        break;
    // the wheel is vertical scrolling, pan is horizontal
    case Mouse_ScrollX:
        axes[MouseState::Pan] = deflection;
        break;
    case Mouse_ScrollY:
        axes[MouseState::Wheel] = deflection;
        break;
    }
}
void MouseAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // TODO: not a thing currently but we could map to the wii cursor maybe
}
void MouseAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void MouseAxisMapping::update_ps2(uint8_t *buf)
{
    // TODO: this does exist
}

void MouseAxisMapping::update_ps3(uint8_t *buf)
{
    // not a thing
}

void MouseAxisMapping::update_ps4(uint8_t *buf)
{
    // not a thing
}

void MouseAxisMapping::update_ps5(uint8_t *buf)
{
    // not a thing
}

void MouseAxisMapping::update_xinput(uint8_t *buf)
{
    // not a thing
}
void MouseAxisMapping::update_ogxbox(uint8_t *buf)
{
    // not a thing
}
void MouseAxisMapping::update_xboxone(uint8_t *buf)
{
}

MouseButtonMapping::MouseButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void MouseButtonMapping::update_hid(uint8_t *buf)
{
    if (!m_last_value)
    {
        return;
    }
    auto &buttons = m_profile->mouse_state.buttons;
    switch (m_mapping.mapping.mapping.mouseButton)
    {
    case Mouse_Left:
        buttons |= MOUSE_BUTTON_LEFT;
        break;
    case Mouse_Middle:
        buttons |= MOUSE_BUTTON_MIDDLE;
        break;
    case Mouse_Right:
        buttons |= MOUSE_BUTTON_RIGHT;
        break;
    }
}
void MouseButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void MouseButtonMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void MouseButtonMapping::update_ps2(uint8_t *buf)
{
    // this one is a thing
}

void MouseButtonMapping::update_ps3(uint8_t *buf)
{
    // not a thing
}

void MouseButtonMapping::update_ps4(uint8_t *buf)
{
    // not a thing
}

void MouseButtonMapping::update_ps5(uint8_t *buf)
{
    // not a thing
}

void MouseButtonMapping::update_xinput(uint8_t *buf)
{
    // not a thing
}
void MouseButtonMapping::update_ogxbox(uint8_t *buf)
{
    // not a thing
}
void MouseButtonMapping::update_xboxone(uint8_t *buf)
{
}

MidiNoteMapping::MidiNoteMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void MidiNoteMapping::update_hid(uint8_t *buf)
{
    if (!m_last_value)
    {
        return;
    }
    const auto &note = m_mapping.mapping.mapping.midiNote;
    uint8_t velocity;
    if (note.has_velocity)
    {
        velocity = note.velocity > 0x7F ? 0x7F : note.velocity ? note.velocity : 1;
    }
    else
    {
        // a hit's strength is its raw analog value, so scale it if the input has been calibrated
        uint16_t strength = m_last_pressure;
        if (m_mapping.max != m_mapping.min)
        {
            strength = calibrate(strength, m_mapping.max, m_mapping.min, m_mapping.deadzone, m_mapping.center, true);
        }
        velocity = midi_velocity(strength);
    }
    m_profile->midi_state.set_note(note.channel, note.note, velocity);
}

MidiControlMapping::MidiControlMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void MidiControlMapping::update_hid(uint8_t *buf)
{
    // set even at rest, as the control has to go back to 0 when it's let go
    const auto &control = m_mapping.mapping.mapping.midiControl;
    m_value = midi_scale(m_calibrated_value, m_value, 7);
    m_profile->midi_state.set_control(control.channel, control.control, m_value);
}

MidiPitchBendMapping::MidiPitchBendMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}

void MidiPitchBendMapping::update_hid(uint8_t *buf)
{
    m_value = midi_scale(m_calibrated_value, m_value, 14);
    m_profile->midi_state.set_pitch_bend(m_mapping.mapping.mapping.midiPitchBend, m_value);
}
