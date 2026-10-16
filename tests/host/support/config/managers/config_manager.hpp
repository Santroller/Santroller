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
    void sync_requested_mode_to_current() { fake_loader::state.calls.push_back("sync_requested_mode_to_current"); }
};
