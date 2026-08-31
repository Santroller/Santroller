#pragma once
#include <memory>
#include <map>
#include <vector>
#include <functional>
#include "devices/base.hpp"
#include "device.pb.h"

class DeviceFactory {
public:
    // Create a device from protobuf, optionally preserving state from previous device
    static std::shared_ptr<Device> create_device(
        const proto_Device& proto_device,
        uint32_t device_id,
        std::shared_ptr<Device> previous_device = nullptr
    );
    
    // Access emulation device state
    static proto_PSXEmulationDevice& get_ps2_emulation_device();
    static proto_WiiEmulationDevice& get_wii_emulation_device();
    
    // Cycle state management
    static void set_cycle_state(int32_t id, int32_t state);
    static int32_t get_cycle_state(int32_t id);
    static void add_last_cycle_state(uint32_t state);
    static void clear_cycle_states();
    static void foreach_cycle_state(std::function<void(int32_t id, int32_t state)> callback);
    
    // Toggle state management
    static void set_toggle_state(int32_t id, bool state);
    static bool get_toggle_state(int32_t id);
    static void clear_toggle_states();
    static void foreach_toggle_state(std::function<void(int32_t id, bool state)> callback);
    
    // Get last cycle states for device creation
    static const std::vector<uint32_t>& get_last_cycle_states();
};
