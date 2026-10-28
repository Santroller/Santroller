#pragma once
// What the fakes in support/config record, so the config tests can see what ConfigLoader::apply did
#include <stdint.h>
#include <memory>
#include <string>
#include <vector>
#include "config.pb.h"

class UsbDevice;

namespace fake_loader
{
// A device as the fake load_device decoded it
struct LoadedDevice
{
    proto_Device device;
    std::vector<int32_t> cycle_values;
};

// A profile as the fake load_profile decoded it
struct LoadedProfile
{
    bool has_opts = false;
    proto_ProfileOpts opts;
    std::vector<proto_Mapping> mappings;
    std::vector<proto_Led> leds;
    size_t assignment_lists = 0;
    std::vector<proto_ProfileAssignmentInfo> assignments;
};

struct State
{
    std::vector<LoadedDevice> devices;
    std::vector<LoadedProfile> profiles;

    // Calls into the managers, in the order apply made them
    std::vector<std::string> calls;
    // The ids apply kept when it dropped the devices the new config doesn't have
    std::vector<uint32_t> kept_root_device_ids;

    // Inputs to apply
    ConsoleMode requested_mode = ModeHid;
    ConsoleMode detected_mode = ModeHid;
    bool has_active_instances = true;
    bool should_reinitialize_device_stack = false;
    bool battery_present_changed = false;
    bool changed_types = false;
    bool mode_from_profile = false;
    bool tool_closed = true;
    std::shared_ptr<UsbDevice> previous_xbox_one_adapter;
    std::shared_ptr<UsbDevice> xbox_one_adapter;

    // Outputs of apply
    bool inactivity_configured = false;
    bool inactivity_had_config = false;
    proto_InactivityConfig inactivity{};
    // nullptr is passed on as "unset", to tell it apart from an empty name
    std::string device_name;
    bool device_name_set = false;
    std::string manufacturer_name;
    bool manufacturer_name_set = false;
    bool xbox_one_legacy_adapter = false;
    bool xbox_one_legacy_adapter_gamepad = true;
    bool loaded_any = false;
    int secondary_pico_inits = 0;
    int reinitialize_device_stack_calls = 0;
    int usb_instances_added = 0;
};

inline State state;

inline void reset() { state = State(); }
}
