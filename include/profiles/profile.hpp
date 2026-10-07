#pragma once
#include <array>
#include <map>
#include <utility>
#include <vector>
#include <memory>
#include "input.pb.h"
#include "config.pb.h"
#include "devices/base.hpp"
#include "input/input.hpp"
#include "triggers/activation_trigger_list.hpp"
#include "profiles/input_queue.hpp"

// Forward declarations to avoid circular dependencies
class Mapping;
class ButtonMapping;
class ActionMapping;
class LedMapping;

enum class DeviceSlotKind : uint8_t
{
    WiiExtension,
    PS2,
    USB,
    Bluetooth,
    MIDI,
    None = 0xff
};

using DeviceSlotKey = std::pair<DeviceSlotKind, uint16_t>;

class Profile
{
public:
    virtual ~Profile();
    void resolve_shortcuts();
    // Drop mappings and LEDs, leaving just what is needed to notice the profile activating
    void release_to_triggers();
    void reset_drum_state();
    // Sample the queued fret / strum mappings and advance the input queue
    void sample_input_queue();
    char name[32];
    SubType subtype;
    bool xinput_on_windows;
    bool invert_y_axis_hid;
    bool supports_ps4;
    bool supports_slider;
    bool cymbal_glitch_fix;
    bool full_range_turntable_on_pc = false;
    bool ps3_on_rpcs3 = true;
    bool disconnect_bluetooth_on_suspend = false;
    bool select_to_dpad_left = false;
    bool select_as_dpad_left() const
    {
        return select_to_dpad_left && (subtype == GuitarHeroGuitar || subtype == RockBandGuitar);
    }
    bool per_kind_slot_ids = false;
    ConsoleMode mode;
    uint32_t profile_id;
    // Inactive profiles only load their opts and triggers; the config tool loads the rest on demand
    bool triggers_only = false;
    std::vector<std::unique_ptr<Mapping>> mappings;
    std::vector<std::unique_ptr<ActivationTriggerList>> triggers;
    std::vector<std::unique_ptr<LedMapping>> leds;
    std::map<uint16_t, std::shared_ptr<Device>> devices;
    std::map<uint16_t, std::shared_ptr<Device>> temp_devices;
    std::map<DeviceSlotKey, std::shared_ptr<Device>> claimed_devices;
    std::map<DeviceSlotKey, std::shared_ptr<Device>> temp_claimed_devices;

    std::shared_ptr<Device> get_claimed_device(DeviceSlotKind kind, uint16_t slot_id, bool temporary = false) const
    {
        const auto &claimed = temporary ? temp_claimed_devices : claimed_devices;
        auto it = claimed.find({kind, slot_id});
        return it == claimed.end() ? nullptr : it->second;
    }

    std::shared_ptr<Device> get_midi_source_device(
        uint16_t slot_id,
        DeviceSlotKind source_kind,
        bool temporary = false) const
    {
        auto supports_midi = [](const std::shared_ptr<Device> &device) {
            return device && (device->is_midi_device() || device->is_pro_guitar_midi_device());
        };

        if (source_kind != DeviceSlotKind::None) {
            auto device = get_claimed_device(source_kind, slot_id, temporary);
            return supports_midi(device) ? device : nullptr;
        }

        std::shared_ptr<Device> matched_device;
        for (auto kind : {DeviceSlotKind::MIDI, DeviceSlotKind::USB, DeviceSlotKind::Bluetooth, DeviceSlotKind::WiiExtension}) {
            auto device = get_claimed_device(kind, slot_id, temporary);
            if (!supports_midi(device)) continue;
            if (matched_device && matched_device != device) return nullptr;
            matched_device = device;
        }
        if (matched_device) return matched_device;

        auto it = devices.find(slot_id);
        return it != devices.end() && supports_midi(it->second) ? it->second : nullptr;
    }

    InputQueue input_queue;
    std::vector<ButtonMapping *> queued_mappings;
    std::vector<bool> queued_last_live;
    std::vector<ActionMapping *> action_mappings;
    bool combined_strum_debounce = false;
    std::vector<ButtonMapping *> strum_mappings;
    // Whether a strum in a direction other than strum_bit is held or still debouncing
    bool other_strum_live(int8_t strum_bit) const;
    void update_actions();
    DrumState drum_state;
    KeyboardState keyboard_state;
    MouseState mouse_state;
    ConsumerState consumer_state;
};