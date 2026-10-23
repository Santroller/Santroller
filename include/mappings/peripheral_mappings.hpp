#pragma once
#include "mappings/base_mapping.hpp"

class WheelButtonMapping : public ButtonMapping
{
public:
    ~WheelButtonMapping() {}
    WheelButtonMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t format, uint8_t *buf);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};

class WheelAxisMapping : public AxisMapping
{
public:
    ~WheelAxisMapping() {}
    WheelAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t format, uint8_t *buf);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};

class KeyboardButtonMapping : public ButtonMapping
{
public:
    ~KeyboardButtonMapping() {}
    KeyboardButtonMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t format, uint8_t *buf);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};

// Sends every key held on a USB keyboard, apart from the keys other mappings read from it (see
// Profile::resolve_keyboard_passthrough), so those can be remapped
class KeyboardPassthroughMapping : public ButtonMapping
{
public:
    ~KeyboardPassthroughMapping() {}
    KeyboardPassthroughMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
    KeyboardPassthroughMapping *as_keyboard_passthrough() override { return this; }
    void clear_masked_keys() { memset(m_masked_keys, 0, sizeof(m_masked_keys)); }
    void mask_key(uint8_t keycode) { m_masked_keys[keycode >> 5] |= 1u << (keycode & 31); }

private:
    uint32_t m_masked_keys[8] = {};
};

class MouseAxisMapping : public AxisMapping
{
public:
    ~MouseAxisMapping() {}
    MouseAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t format, uint8_t *buf);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};

class MouseButtonMapping : public ButtonMapping
{
public:
    ~MouseButtonMapping() {}
    MouseButtonMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t format, uint8_t *buf);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};

// A MIDI note, held while the input is
class MidiNoteMapping : public ButtonMapping
{
public:
    ~MidiNoteMapping() {}
    MidiNoteMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
};

// A MIDI control change, following an axis
class MidiControlMapping : public AxisMapping
{
public:
    ~MidiControlMapping() {}
    MidiControlMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);

private:
    uint16_t m_value = 0;
};

// A channel's MIDI pitch bend, following an axis
class MidiPitchBendMapping : public AxisMapping
{
public:
    ~MidiPitchBendMapping() {}
    MidiPitchBendMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);

private:
    uint16_t m_value = MIDI_OUTPUT_PITCH_BEND_CENTER;
};
