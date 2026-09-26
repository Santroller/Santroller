#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/base.hpp"
#include "devices/midi.hpp"
#include "profiles/profile.hpp"
#include <memory>
class MidiNoteInput : public Input
{
public:
    MidiNoteInput(proto_MidiInput input, std::shared_ptr<MidiDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool consumes_events() const override { return true; }
    bool consume_event(uint16_t &value) override;
    MidiNoteInput *as_midi_note() override { return this; }
    uint8_t channel() const { return m_input.channel; }
    uint8_t note() const { return m_input.note; }
    std::shared_ptr<MidiDevice> device() const { return m_device; }
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_MidiNote) << 56) | (static_cast<uint64_t>(m_device ? m_device->m_id : 0) << 16) | (static_cast<uint64_t>(m_input.channel) << 8) | static_cast<uint64_t>(m_input.note); }
    void link_device(bool claim_devices) override
    {
        if (claim_devices) {
            m_device = std::static_pointer_cast<MidiDevice>(m_profile->devices[m_device_id]);
        } else {
            m_device = std::static_pointer_cast<MidiDevice>(m_profile->temp_devices[m_device_id]);
        }
    };

private:
    void setup();
    proto_MidiNoteInput m_input;
    std::shared_ptr<MidiDevice> m_device;
    Profile *m_profile;
    uint16_t m_last_event_sequence = 0;
    uint32_t m_device_id;
};
class MidiControlChangeInput : public Input
{
public:
    MidiControlChangeInput(proto_MidiInput input, std::shared_ptr<MidiDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    void link_device(bool claim_devices) override
    {
        if (claim_devices) {
            m_device = std::static_pointer_cast<MidiDevice>(m_profile->devices[m_device_id]);
        } else {
            m_device = std::static_pointer_cast<MidiDevice>(m_profile->temp_devices[m_device_id]);
        }
    };

private:
    void setup();
    proto_MidiControlChangeInput m_input;
    std::shared_ptr<MidiDevice> m_device;
    Profile *m_profile;
    uint32_t m_device_id;
};
class MidiPitchBendInput : public Input
{
public:
    MidiPitchBendInput(proto_MidiInput input, std::shared_ptr<MidiDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    void link_device(bool claim_devices) override
    {
        if (claim_devices) {
            m_device = std::static_pointer_cast<MidiDevice>(m_profile->devices[m_device_id]);
        } else {
            m_device = std::static_pointer_cast<MidiDevice>(m_profile->temp_devices[m_device_id]);
        }
    };

private:
    void setup();
    proto_MidiPitchBendInput m_input;
    std::shared_ptr<MidiDevice> m_device;
    Profile *m_profile;
    uint32_t m_device_id;
};
class MidiProGuitarButtonInput : public Input
{
public:
    MidiProGuitarButtonInput(proto_MidiInput input, std::shared_ptr<ProGuitarMidiDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    void link_device(bool claim_devices) override
    {
        if (claim_devices) {
            m_device = std::static_pointer_cast<ProGuitarMidiDevice>(m_profile->devices[m_device_id]);
        } else {
            m_device = std::static_pointer_cast<ProGuitarMidiDevice>(m_profile->temp_devices[m_device_id]);
        }
    };

private:
    void setup();
    proto_MidiProGuitarButtonInput m_input;
    std::shared_ptr<ProGuitarMidiDevice> m_device;
    Profile *m_profile;
    uint32_t m_device_id;
};
class MidiProGuitarAxisInput : public Input
{
public:
    MidiProGuitarAxisInput(proto_MidiInput input, std::shared_ptr<ProGuitarMidiDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    void link_device(bool claim_devices) override
    {
        if (claim_devices) {
            m_device = std::static_pointer_cast<ProGuitarMidiDevice>(m_profile->devices[m_device_id]);
        } else {
            m_device = std::static_pointer_cast<ProGuitarMidiDevice>(m_profile->temp_devices[m_device_id]);
        }
    };

private:
    void setup();
    proto_MidiProGuitarAxisInput m_input;
    std::shared_ptr<ProGuitarMidiDevice> m_device;
    Profile *m_profile;
    uint32_t m_device_id;
};