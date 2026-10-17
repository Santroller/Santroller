#pragma once
#include <memory>
#include <stdint.h>
#include "commands.pb.h"
#include "events.pb.h"
#include "profiles/profile.hpp"

// Shared by pin and input detection
// An input has to stay changed this long before detection takes it
static constexpr uint32_t DETECT_HOLD_MS = 30;
// Changes closer together than this count as one, so a bouncy button isn't mistaken for noise
static constexpr uint32_t DETECT_BOUNCE_MS = 10;
// An input that changes more often than this without being held is ignored for the rest of the detect
static constexpr uint8_t DETECT_NOISY_TRANSITIONS = 5;
// How far an axis has to move from where it started, out of 65535
static constexpr uint16_t DETECT_AXIS_THRESHOLD = 0x4000;
// The same for MIDI control changes (out of 127) and pitch bends (out of 16383)
static constexpr uint16_t DETECT_MIDI_CONTROL_THRESHOLD = 32;
static constexpr uint16_t DETECT_MIDI_PITCH_BEND_THRESHOLD = 0x1000;

// Finds which input of a device (a wii extension, usb host controller etc) the user presses or moves,
// by reading every button and axis it has and taking the first that leaves where it started
class InputDetector
{
public:
    void begin(const proto_DetectInputCommand &command, uint32_t timeout_at);
    void end() { m_active = false; }
    bool active() const { return m_active; }
    // Reads the device through the profile, as its inputs would. True once an input is found, filling in event
    bool poll(const std::shared_ptr<Profile> &profile, proto_InputDetectionEvent &event);

    // Every enum value of the axis and button types is tried, and none go past this
    static constexpr uint8_t MAX_VALUE = 72;

private:
    struct Candidate
    {
        uint16_t baseline;
        uint32_t changed_at;
        uint8_t transitions;
        bool changed;
        bool valid;
        bool noisy;
    };
    struct MidiStart
    {
        uint8_t channel;
        // The control change number, or MIDI_START_PITCH_BEND
        uint8_t number;
        uint16_t value;
    };
    static constexpr uint8_t MIDI_START_PITCH_BEND = 0x80;
    static constexpr uint8_t MAX_MIDI_STARTS = 16;

    std::shared_ptr<Device> find_device(const std::shared_ptr<Profile> &profile);
    bool read(bool axis, uint8_t value, uint16_t &out);
    bool read_key(uint8_t keycode);
    void capture_baseline();
    bool check(bool axis, uint8_t value, uint32_t now);
    bool poll_keys(proto_InputDetectionEvent &event);
    bool poll_midi(proto_InputDetectionEvent &event);
    uint16_t midi_start_value(uint8_t channel, uint8_t number, uint16_t previous);

    proto_DetectInputCommand m_command;
    std::shared_ptr<Device> m_device;
    uint32_t m_timeout_at = 0;
    uint8_t m_button_max = 0;
    uint8_t m_axis_max = 0;
    bool m_active = false;
    Candidate m_buttons[MAX_VALUE + 1];
    Candidate m_axes[MAX_VALUE + 1];
    // Keyboard keys held when the detect started, by HID keycode
    uint32_t m_keys_held[8];
    // MIDI: how far through the device's note events and changes the detect has looked, and the
    // value each control moved so far started at
    uint16_t m_midi_note_sequence;
    uint16_t m_midi_control_sequence;
    uint16_t m_midi_pitch_bend_sequence;
    MidiStart m_midi_starts[MAX_MIDI_STARTS];
    uint8_t m_midi_start_count;
};
