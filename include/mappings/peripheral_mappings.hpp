#pragma once
#include "mappings/base_mapping.hpp"

class WheelButtonMapping : public ButtonMapping
{
public:
    ~WheelButtonMapping() {}
    WheelButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
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
    WheelAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
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
    KeyboardButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
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

class MouseAxisMapping : public AxisMapping
{
public:
    ~MouseAxisMapping() {}
    MouseAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
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
    MouseButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
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
    MidiNoteMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
};

// A MIDI control change, following an axis
class MidiControlMapping : public AxisMapping
{
public:
    ~MidiControlMapping() {}
    MidiControlMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);

private:
    uint16_t m_value = 0;
};

// A channel's MIDI pitch bend, following an axis
class MidiPitchBendMapping : public AxisMapping
{
public:
    ~MidiPitchBendMapping() {}
    MidiPitchBendMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);

private:
    uint16_t m_value = MIDI_OUTPUT_PITCH_BEND_CENTER;
};
