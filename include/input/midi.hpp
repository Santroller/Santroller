#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/base.hpp"
#include "devices/midi.hpp"
#include "profiles/profile.hpp"
#include <memory>

inline DeviceSlotKind midi_input_slot_kind(const proto_MidiInput &input)
{
    if (!input.has_sourceType) {
        return DeviceSlotKind::None;
    }
    switch (input.sourceType) {
    case proto_MidiInputSourceType_MidiInputSourceType_MIDI:
        return DeviceSlotKind::MIDI;
    case proto_MidiInputSourceType_MidiInputSourceType_USB:
        return DeviceSlotKind::USB;
    case proto_MidiInputSourceType_MidiInputSourceType_Bluetooth:
        return DeviceSlotKind::Bluetooth;
    case proto_MidiInputSourceType_MidiInputSourceType_Wii:
        return DeviceSlotKind::WiiExtension;
    default:
        return DeviceSlotKind::None;
    }
}

class MidiNoteInput : public Input
{
public:
    MidiNoteInput(proto_MidiInput input, std::shared_ptr<MidiDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool consumes_events() const override { return true; }
    bool consume_event(uint16_t &value) override;
    bool peek_event(uint16_t &value) override;
    MidiNoteInput *as_midi_note() override { return this; }
    uint8_t channel() const { return m_input.channel; }
    uint8_t note() const { return m_input.note; }
    std::shared_ptr<MidiDevice> device() const { return m_device; }
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_MidiNote) << 56) | (static_cast<uint64_t>(m_device ? m_device->m_id : 0) << 16) | (static_cast<uint64_t>(m_input.channel) << 8) | static_cast<uint64_t>(m_input.note); }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_midi_source_device(m_device_id, m_slot_kind, !claim_devices);
        m_device = device && device->is_midi_device() ? std::static_pointer_cast<MidiDevice>(device) : nullptr;
    };
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_MidiNoteInput m_input;
    std::shared_ptr<MidiDevice> m_device;
    Profile *m_profile;
    uint16_t m_last_event_sequence = 0;
    uint16_t m_last_ui_event_sequence = 0;
    uint32_t m_device_id;
    DeviceSlotKind m_slot_kind;
};
class MidiControlChangeInput : public Input
{
public:
    MidiControlChangeInput(proto_MidiInput input, std::shared_ptr<MidiDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_midi_source_device(m_device_id, m_slot_kind, !claim_devices);
        m_device = device && device->is_midi_device() ? std::static_pointer_cast<MidiDevice>(device) : nullptr;
    };
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_MidiControlChangeInput m_input;
    std::shared_ptr<MidiDevice> m_device;
    Profile *m_profile;
    uint32_t m_device_id;
    DeviceSlotKind m_slot_kind;
};
class MidiPitchBendInput : public Input
{
public:
    MidiPitchBendInput(proto_MidiInput input, std::shared_ptr<MidiDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_midi_source_device(m_device_id, m_slot_kind, !claim_devices);
        m_device = device && device->is_midi_device() ? std::static_pointer_cast<MidiDevice>(device) : nullptr;
    };
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_MidiPitchBendInput m_input;
    std::shared_ptr<MidiDevice> m_device;
    Profile *m_profile;
    uint32_t m_device_id;
    DeviceSlotKind m_slot_kind;
};
class MidiProGuitarButtonInput : public Input
{
public:
    MidiProGuitarButtonInput(proto_MidiInput input, std::shared_ptr<ProGuitarMidiDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_midi_source_device(m_device_id, m_slot_kind, !claim_devices);
        m_device = device && device->is_pro_guitar_midi_device()
            ? std::static_pointer_cast<ProGuitarMidiDevice>(device)
            : nullptr;
    };
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_MidiProGuitarButtonInput m_input;
    std::shared_ptr<ProGuitarMidiDevice> m_device;
    Profile *m_profile;
    uint32_t m_device_id;
    DeviceSlotKind m_slot_kind;
};
class MidiProGuitarAxisInput : public Input
{
public:
    MidiProGuitarAxisInput(proto_MidiInput input, std::shared_ptr<ProGuitarMidiDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_midi_source_device(m_device_id, m_slot_kind, !claim_devices);
        m_device = device && device->is_pro_guitar_midi_device()
            ? std::static_pointer_cast<ProGuitarMidiDevice>(device)
            : nullptr;
    };
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_MidiProGuitarAxisInput m_input;
    std::shared_ptr<ProGuitarMidiDevice> m_device;
    Profile *m_profile;
    uint32_t m_device_id;
    DeviceSlotKind m_slot_kind;
};