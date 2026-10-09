#pragma once
// Fake managers/battery_manager.hpp
#include "config_fakes.hpp"

class BatteryManager
{
public:
    static BatteryManager &instance()
    {
        static BatteryManager manager;
        return manager;
    }
    void begin_config_reload() { fake_loader::state.calls.push_back("battery_begin_config_reload"); }
    bool present_changed() const { return fake_loader::state.battery_present_changed; }
};
