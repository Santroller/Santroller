#include "protocols/motion.hpp"
#include "config/input_detector.hpp"
#include "config/input_factory.hpp"
#include "devices/wii.hpp"
#include "devices/ps2.hpp"
#include "devices/snes.hpp"
#include "devices/joybus.hpp"
#include "devices/usb/host/host.hpp"
#include "devices/bt/bt_host.hpp"
#include "devices/midi.hpp"
#include "input/midi.hpp"
#include "input_enums.pb.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

// Builds the Output a USB or bluetooth host reads, for the Output fields that hold an enum
static bool make_output(uint32_t tag, uint8_t value, proto_Output &output)
{
    output = {};
    output.which_mapping = tag;
    switch (tag)
    {
#define DETECT_OUTPUT(field, type)   \
    case proto_Output_##field##_tag: \
        output.mapping.field = static_cast<type>(value); \
        return true;
        DETECT_OUTPUT(gamepadButton, proto_GamepadButtonType)
        DETECT_OUTPUT(gamepadAxis, proto_GamepadAxisType)
        DETECT_OUTPUT(ghButton, proto_GuitarHeroGuitarButtonType)
        DETECT_OUTPUT(ghAxis, proto_GuitarHeroGuitarAxisType)
        DETECT_OUTPUT(rbButton, proto_RockBandGuitarButtonType)
        DETECT_OUTPUT(rbAxis, proto_RockBandGuitarAxisType)
        DETECT_OUTPUT(ghDrumAxis, proto_GuitarHeroDrumsAxisType)
        DETECT_OUTPUT(rbDrumButton, proto_RockBandDrumsButtonType)
        DETECT_OUTPUT(rbDrumAxis, proto_RockBandDrumsAxisType)
        DETECT_OUTPUT(ghlButton, proto_GuitarHeroLiveGuitarButtonType)
        DETECT_OUTPUT(ghlAxis, proto_GuitarHeroLiveGuitarAxisType)
        DETECT_OUTPUT(proButton, proto_ProGuitarButtonType)
        DETECT_OUTPUT(proAxis, proto_ProGuitarAxisType)
        DETECT_OUTPUT(djhButton, proto_DJHTurntableButtonType)
        DETECT_OUTPUT(djhAxis, proto_DJHTurntableAxisType)
        DETECT_OUTPUT(djMaxButton, proto_DJMaxTurntableButtonType)
        DETECT_OUTPUT(djMaxAxis, proto_DJMaxTurntableAxisType)
        DETECT_OUTPUT(divaAxis, proto_ProjectDivaAxisType)
        DETECT_OUTPUT(gfButton, proto_GuitarFreaksButtonType)
        DETECT_OUTPUT(ghaAxis, proto_GuitarHeroArcadeAxisType)
        DETECT_OUTPUT(ghaButton, proto_GuitarHeroArcadeButtonType)
        DETECT_OUTPUT(proKeyboardAxis, proto_ProKeyboardAxisType)
        DETECT_OUTPUT(proKeyboardButton, proto_ProKeyboardButtonType)
        DETECT_OUTPUT(popnButton, proto_PopNMusicButtonType)
        DETECT_OUTPUT(bmButton, proto_BeatManiaButtonType)
        DETECT_OUTPUT(dmButton, proto_DrumManiaButtonType)
        DETECT_OUTPUT(kbmButton, proto_KeyboardManiaButtonType)
        DETECT_OUTPUT(mouseButton, proto_MouseButtonType)
        DETECT_OUTPUT(mouseAxis, proto_MouseAxisType)
#undef DETECT_OUTPUT
    default:
        return false;
    }
}

void InputDetector::begin(const proto_DetectInputCommand &command, uint32_t timeout_at)
{
    m_command = command;
    m_timeout_at = timeout_at;
    m_device = nullptr;
    m_active = true;
    switch (command.kind)
    {
    case DetectWii:
        m_button_max = _WiiButtonType_MAX;
        m_axis_max = _WiiAxisType_MAX;
        break;
    case DetectPS2:
        m_button_max = _PS2ButtonType_MAX;
        m_axis_max = _PS2AxisType_MAX;
        break;
    case DetectSNES:
        m_button_max = _SNESButtonType_MAX;
        m_axis_max = _SNESAxisType_MAX;
        break;
    case DetectJoybus:
        m_button_max = _JoybusButtonType_MAX;
        m_axis_max = _JoybusAxisType_MAX;
        break;
    case DetectUsb:
    case DetectBluetooth:
        m_button_max = command.has_buttonOutput && command.has_buttonOutputMax ? command.buttonOutputMax : 0;
        m_axis_max = command.has_axisOutput && command.has_axisOutputMax ? command.axisOutputMax : 0;
        break;
    default:
        m_button_max = m_axis_max = 0;
        break;
    }
    m_button_max = std::min(m_button_max, MAX_VALUE);
    m_axis_max = command.axes ? std::min(m_axis_max, MAX_VALUE) : 0;
}

std::shared_ptr<Device> InputDetector::find_device(const std::shared_ptr<Profile> &profile)
{
    uint32_t id = m_command.deviceid;
    switch (m_command.kind)
    {
    case DetectWii:
        return InputFactory::get_device<WiiDevice>(profile, id);
    case DetectPS2:
        return InputFactory::get_device<PS2Device>(profile, id);
    case DetectSNES:
        return InputFactory::get_device<SNESDevice>(profile, id);
    case DetectJoybus:
        return InputFactory::get_device<JoybusDevice>(profile, id);
    case DetectUsb:
        return InputFactory::get_device<UsbHostInterface>(profile, id);
    case DetectBluetooth:
        return InputFactory::get_device<BluetoothHostInterface>(profile, id);
    case DetectMidi:
    {
        // Found the same way as the MIDI input it'll become
        proto_MidiInput input = proto_MidiInput_init_zero;
        input.has_sourceType = m_command.has_midiSourceType;
        input.sourceType = m_command.midiSourceType;
        return profile->get_midi_source_device(static_cast<uint16_t>(id), midi_input_slot_kind(input));
    }
    default:
        return nullptr;
    }
}

bool InputDetector::read_key(uint8_t keycode)
{
    proto_Output output = {};
    output.which_mapping = proto_Output_keycode_tag;
    output.mapping.keycode = keycode;
    if (m_command.kind == DetectUsb)
    {
        return std::static_pointer_cast<UsbHostInterface>(m_device)->tick_digital(output);
    }
    return std::static_pointer_cast<BluetoothHostInterface>(m_device)->tick_digital(output);
}

bool InputDetector::read(bool axis, uint8_t value, uint16_t &out)
{
    switch (m_command.kind)
    {
    case DetectWii:
    {
        auto device = std::static_pointer_cast<WiiDevice>(m_device);
        out = axis ? device->read_axis(static_cast<proto_WiiAxisType>(value)) : device->read_button(static_cast<proto_WiiButtonType>(value));
        return true;
    }
    case DetectPS2:
    {
        auto device = std::static_pointer_cast<PS2Device>(m_device);
        out = axis ? device->read_axis(static_cast<proto_PS2AxisType>(value)) : device->read_button(static_cast<proto_PS2ButtonType>(value));
        return true;
    }
    case DetectSNES:
    {
        auto device = std::static_pointer_cast<SNESDevice>(m_device);
        out = axis ? device->read_axis(static_cast<proto_SNESAxisType>(value)) : device->read_button(static_cast<proto_SNESButtonType>(value));
        return true;
    }
    case DetectJoybus:
    {
        auto device = std::static_pointer_cast<JoybusDevice>(m_device);
        out = axis ? device->read_axis(static_cast<proto_JoybusAxisType>(value)) : device->read_button(static_cast<proto_JoybusButtonType>(value));
        return true;
    }
    case DetectUsb:
    case DetectBluetooth:
    {
        proto_Output output;
        if (!make_output(axis ? m_command.axisOutput : m_command.buttonOutput, value, output))
        {
            return false;
        }
        // motion moves whenever the controller does, so it would win over whatever was meant
        if (output.which_mapping == proto_Output_gamepadAxis_tag && is_motion_axis(output.mapping.gamepadAxis))
        {
            return false;
        }
        if (m_command.kind == DetectUsb)
        {
            auto device = std::static_pointer_cast<UsbHostInterface>(m_device);
            out = axis ? device->tick_analog(output) : device->tick_digital(output);
        }
        else
        {
            auto device = std::static_pointer_cast<BluetoothHostInterface>(m_device);
            out = axis ? device->tick_analog(output) : device->tick_digital(output);
        }
        return true;
    }
    default:
        return false;
    }
}

void InputDetector::capture_baseline()
{
    memset(m_buttons, 0, sizeof(m_buttons));
    memset(m_axes, 0, sizeof(m_axes));
    for (uint8_t value = 1; value <= m_button_max; value++)
    {
        m_buttons[value].valid = read(false, value, m_buttons[value].baseline);
    }
    for (uint8_t value = 1; value <= m_axis_max; value++)
    {
        m_axes[value].valid = read(true, value, m_axes[value].baseline);
    }
    memset(m_keys_held, 0, sizeof(m_keys_held));
    if (m_command.keys && (m_command.kind == DetectUsb || m_command.kind == DetectBluetooth))
    {
        for (uint16_t key = 1; key <= UINT8_MAX; key++)
        {
            if (read_key(key))
            {
                m_keys_held[key / 32] |= 1u << (key % 32);
            }
        }
    }
    if (m_command.kind == DetectMidi)
    {
        const MidiInput &midi = std::static_pointer_cast<MidiDevice>(m_device)->midi_state();
        m_midi_note_sequence = midi.midi_note_event_sequence();
        m_midi_control_sequence = midi.last_control_change().sequence;
        m_midi_pitch_bend_sequence = midi.last_pitch_bend().sequence;
        m_midi_start_count = 0;
    }
}

// Keys don't bounce or misfire, so take the first one pressed that wasn't held at the start
bool InputDetector::poll_keys(proto_InputDetectionEvent &event)
{
    for (uint16_t key = 1; key <= UINT8_MAX; key++)
    {
        uint32_t bit = 1u << (key % 32);
        bool pressed = read_key(key);
        if (m_keys_held[key / 32] & bit)
        {
            // Let go of since the start, so pressing it again counts
            if (!pressed)
            {
                m_keys_held[key / 32] &= ~bit;
            }
            continue;
        }
        if (pressed)
        {
            event.axis = false;
            event.key = true;
            event.has_key = true;
            event.value = key;
            return true;
        }
    }
    return false;
}

// The value a control had before the detect first saw it move
uint16_t InputDetector::midi_start_value(uint8_t channel, uint8_t number, uint16_t previous)
{
    for (uint8_t i = 0; i < m_midi_start_count; i++)
    {
        if (m_midi_starts[i].channel == channel && m_midi_starts[i].number == number)
        {
            return m_midi_starts[i].value;
        }
    }
    if (m_midi_start_count < MAX_MIDI_STARTS)
    {
        m_midi_starts[m_midi_start_count++] = {channel, number, previous};
    }
    return previous;
}

// Notes are hits (a drum pad, say) that are over by the next poll, so take any note on straight away.
// Controls (a hi-hat pedal, a mod wheel) send a stream of changes, so they have to move a fair way
bool InputDetector::poll_midi(proto_InputDetectionEvent &event)
{
    const MidiInput &midi = std::static_pointer_cast<MidiDevice>(m_device)->midi_state();
    uint8_t channel;
    uint8_t note;
    event.has_midiType = true;
    event.has_midiChannel = true;
    if (midi.next_midi_note_event(m_midi_note_sequence, channel, note))
    {
        event.axis = false;
        event.midiType = DetectMidiNote;
        event.midiChannel = channel + 1;
        event.value = note;
        return true;
    }
    const auto &control = midi.last_control_change();
    if (control.sequence != m_midi_control_sequence)
    {
        m_midi_control_sequence = control.sequence;
        uint16_t start = midi_start_value(control.channel, control.number, control.previous);
        if (abs(control.value - start) >= DETECT_MIDI_CONTROL_THRESHOLD)
        {
            event.axis = true;
            event.midiType = DetectMidiControlChange;
            event.midiChannel = control.channel + 1;
            event.value = control.number;
            return true;
        }
    }
    const auto &bend = midi.last_pitch_bend();
    if (bend.sequence != m_midi_pitch_bend_sequence)
    {
        m_midi_pitch_bend_sequence = bend.sequence;
        uint16_t start = midi_start_value(bend.channel, MIDI_START_PITCH_BEND, bend.previous);
        if (abs(bend.value - start) >= DETECT_MIDI_PITCH_BEND_THRESHOLD)
        {
            event.axis = true;
            event.midiType = DetectMidiPitchBend;
            event.midiChannel = bend.channel + 1;
            event.value = 0;
            return true;
        }
    }
    event.has_midiType = false;
    event.has_midiChannel = false;
    return false;
}

// Same rules as pin detection: the input has to be held away from where it started, and one
// that keeps flicking (a misfiring tilt sensor, say) is dropped so it can't win every detect
bool InputDetector::check(bool axis, uint8_t value, uint32_t now)
{
    Candidate &candidate = axis ? m_axes[value] : m_buttons[value];
    if (!candidate.valid || candidate.noisy)
    {
        return false;
    }
    uint16_t current;
    if (!read(axis, value, current))
    {
        return false;
    }
    // Buttons only count when pressed, so letting go of one held at the start doesn't
    bool moved = axis ? abs(current - candidate.baseline) >= DETECT_AXIS_THRESHOLD : (current && !candidate.baseline);
    if (!moved)
    {
        candidate.changed = false;
        return false;
    }
    if (!candidate.changed)
    {
        if (now - candidate.changed_at >= DETECT_BOUNCE_MS && ++candidate.transitions > DETECT_NOISY_TRANSITIONS)
        {
            printf("ignoring noisy %s: %d\r\n", axis ? "axis" : "button", value);
            candidate.noisy = true;
        }
        candidate.changed = true;
        candidate.changed_at = now;
        return false;
    }
    return now - candidate.changed_at >= DETECT_HOLD_MS;
}

bool InputDetector::poll(const std::shared_ptr<Profile> &profile, proto_InputDetectionEvent &event)
{
    if (!m_active)
    {
        return false;
    }
    uint32_t now = millis();
    if (now > m_timeout_at)
    {
        m_active = false;
        return false;
    }
    // The device may not be connected or claimed yet, so keep looking for it until it is
    if (!m_device)
    {
        m_device = find_device(profile);
        if (!m_device)
        {
            return false;
        }
        capture_baseline();
        return false;
    }
    if (m_command.kind == DetectMidi)
    {
        if (poll_midi(event))
        {
            m_active = false;
            return true;
        }
        return false;
    }
    if (m_command.keys && (m_command.kind == DetectUsb || m_command.kind == DetectBluetooth) && poll_keys(event))
    {
        m_active = false;
        return true;
    }
    // Buttons first, as pressing one can nudge an axis (an accelerometer, say) at the same time
    for (uint8_t value = 1; value <= m_button_max; value++)
    {
        if (check(false, value, now))
        {
            event.axis = false;
            event.value = value;
            m_active = false;
            return true;
        }
    }
    for (uint8_t value = 1; value <= m_axis_max; value++)
    {
        if (check(true, value, now))
        {
            event.axis = true;
            event.value = value;
            m_active = false;
            return true;
        }
    }
    return false;
}
