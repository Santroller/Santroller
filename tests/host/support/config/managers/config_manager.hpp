#pragma once
// Fake managers/config_manager.hpp: records what ConfigLoader asks of it
#include "config_fakes.hpp"

class ConfigManager
{
public:
    static ConfigManager &instance()
    {
        static ConfigManager manager;
        return manager;
    }
    void clear_seen_masks() { fake_loader::state.calls.push_back("clear_seen_masks"); }
    void clear_deferred_forced_mode() {}
    bool is_mode_from_profile() const { return fake_loader::state.mode_from_profile; }
    void set_mode_from_profile(bool forced) { fake_loader::state.mode_from_profile = forced; }
    void request_mode(ConsoleMode mode)
    {
        fake_loader::state.calls.push_back("request_mode");
        fake_loader::state.requested_mode = mode;
    }
    ConsoleMode get_requested_mode() const { return fake_loader::state.requested_mode; }
    ConsoleMode get_detected_mode() const { return fake_loader::state.detected_mode; }
    bool should_reinitialize_device_stack() const { return fake_loader::state.should_reinitialize_device_stack; }
    void request_device_stack_reinit() { fake_loader::state.should_reinitialize_device_stack = true; }
    void set_device_name(const char *name)
    {
        fake_loader::state.device_name_set = name != nullptr;
        fake_loader::state.device_name = name ? name : "";
    }
    void set_manufacturer_name(const char *name)
    {
        fake_loader::state.manufacturer_name_set = name != nullptr;
        fake_loader::state.manufacturer_name = name ? name : "";
    }
    void set_xbox_one_legacy_adapter(bool legacy) { fake_loader::state.xbox_one_legacy_adapter = legacy; }
    bool xbox_one_legacy_adapter_gamepad() const { return fake_loader::state.xbox_one_legacy_adapter_gamepad; }
    void set_xbox_one_legacy_adapter_gamepad(bool gamepad) { fake_loader::state.xbox_one_legacy_adapter_gamepad = gamepad; }
    bool has_loaded_any() const { return fake_loader::state.loaded_any; }
    bool is_rpcs3_detected() const { return false; }
    void clear_rpcs3_detected() {}
    void sync_requested_mode_to_current() { fake_loader::state.calls.push_back("sync_requested_mode_to_current"); }
};
