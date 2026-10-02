#pragma once
#include <map>
#include <utility>
#include <vector>
#include <memory>
#include "input.pb.h"
#include "config.pb.h"
#include "devices/base.hpp"
#include "input/input.hpp"
#include "triggers/activation_trigger_list.hpp"

// Forward declarations to avoid circular dependencies
class Mapping;
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
    void reset_drum_state() { drum_state.reset(); }
    char name[32];
    SubType subtype;
    bool xinput_on_windows;
    bool invert_y_axis_hid;
    bool supports_ps4;
    bool supports_slider;
    bool cymbal_glitch_fix;
    ConsoleMode mode;
    uint32_t profile_id;
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

    DrumState drum_state;
    KeyboardState keyboard_state;
};