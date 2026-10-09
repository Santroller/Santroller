#pragma once
// Fake managers/device_manager.hpp: records what ConfigLoader asks of it
#include "config_fakes.hpp"

class DeviceManager
{
public:
    static DeviceManager &instance()
    {
        static DeviceManager manager;
        return manager;
    }
    void clear_assignable_devices() { fake_loader::state.calls.push_back("clear_assignable_devices"); }
    void clear_active_devices() { fake_loader::state.calls.push_back("clear_active_devices"); }
    void mark_root_devices_disconnected() { fake_loader::state.calls.push_back("mark_root_devices_disconnected"); }
    void remove_disconnected_root_devices() { fake_loader::state.calls.push_back("remove_disconnected_root_devices"); }
};
