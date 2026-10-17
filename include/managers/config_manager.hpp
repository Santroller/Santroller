#pragma once
#include <stdint.h>
#include <string.h>
#include <vector>
#include "config.pb.h"

class Profile;

// A startup trigger that matched. Inputs read from a slot also remember the device they read, so
// the latch doesn't carry over to another device that fills the same slot
struct StartupTriggerKey
{
    uint32_t profile_id;
    uint16_t list_id;
    uint16_t trigger_id;
    uint32_t device_serial;
    bool operator==(const StartupTriggerKey &other) const
    {
        return profile_id == other.profile_id && list_id == other.list_id &&
               trigger_id == other.trigger_id && device_serial == other.device_serial;
    }
};

class ConfigManager
{
public:
    static ConfigManager& instance()
    {
        static ConfigManager manager;
        return manager;
    }

    // Console mode management
    ConsoleMode get_current_mode() const { return m_current_mode; }
    void set_current_mode(ConsoleMode mode) { m_current_mode = mode; }
    ConsoleMode get_requested_mode() const { return m_requested_mode; }
    void request_mode(ConsoleMode mode) { m_requested_mode = mode; }
    bool has_mode_changed() const { return m_mode_changed; }
    void set_mode_changed(bool changed) { m_mode_changed = changed; }
    bool mode_recently_changed(uint32_t now, uint32_t window_ms = 2000) const;
    void mark_mode_change_time(uint32_t now) { m_time_since_mode = now; }
    void sync_requested_mode_to_current() { m_current_mode = m_requested_mode; }
    // The host detected from how it enumerates us (ModeHid for a PC or a host not detected yet). It's
    // kept across reloads, so a reload goes straight back to the host's mode instead of starting over
    // in ModeHid and being detected all over again.
    ConsoleMode get_detected_mode() const { return m_detected_mode; }
    void host_detected(ConsoleMode mode);
    
    // Config loading state
    bool is_working() const { return m_working; }
    void set_working(bool working) { m_working = working; }
    // Note that a config write is still making progress
    void touch_working(uint32_t now)
    {
        m_working = true;
        m_last_work = now;
    }
    bool working_stalled(uint32_t now, uint32_t timeout_ms) const { return m_working && now - m_last_work > timeout_ms; }
    bool has_loaded_any() const { return m_loaded_any; }
    void set_loaded_any(bool loaded) { m_loaded_any = loaded; }
    // What the whole device is called (Config.deviceName), or nullptr when it isn't named
    const char *get_device_name() const { return m_device_name[0] ? m_device_name : nullptr; }
    void set_device_name(const char *name)
    {
        strncpy(m_device_name, name ? name : "", sizeof(m_device_name) - 1);
        m_device_name[sizeof(m_device_name) - 1] = '\0';
    }
    bool has_bluetooth() const { return m_has_bluetooth; }
    void set_bluetooth_available(bool available) { m_has_bluetooth = available; }
    bool is_reloading() const { return m_reloading; }
    void set_reloading(bool reloading) { m_reloading = reloading; }
    bool should_reinit(uint32_t now) const;
    void begin_reinit();
    bool is_full_reload() const { return m_full_reload; }
    void set_full_reload(bool full_reload) { m_full_reload = full_reload; }
    bool should_reinitialize_device_stack() const { return m_reinitialize_device_stack; }
    void request_device_stack_reinit() { m_reinitialize_device_stack = true; }
    // Restart just the device stack from the main loop, without reloading the config
    void request_device_stack_restart() { m_restart_device_stack = true; }
    bool take_device_stack_restart()
    {
        bool requested = m_restart_device_stack;
        m_restart_device_stack = false;
        return requested;
    }
    void finish_reinit(uint32_t now);
    uint32_t get_reinit_time() const { return m_reinit_time; }
    void schedule_reinit(uint32_t when) { m_reinit_time = when; }
    void clear_reinit() { m_reinit_time = 0; }
    // A profile forces a console mode, but it was skipped as the tool was open
    bool is_forced_mode_deferred() const { return m_forced_mode_deferred; }
    ConsoleMode get_deferred_mode() const { return m_deferred_mode; }
    void defer_forced_mode(ConsoleMode mode)
    {
        m_forced_mode_deferred = true;
        m_deferred_mode = mode;
    }
    void clear_deferred_forced_mode() { m_forced_mode_deferred = false; }
    // The requested mode came from a profile forcing it, rather than from detecting the host
    bool is_mode_from_profile() const { return m_mode_from_profile; }
    void set_mode_from_profile(bool forced) { m_mode_from_profile = forced; }
    // RPCS3 probed us, so profiles with ps3_on_rpcs3 run in PS3 mode. Kept across reloads, as the
    // profile picks the mode again on each load, like one that forces it
    bool is_rpcs3_detected() const { return m_rpcs3_detected; }
    void rpcs3_detected()
    {
        m_rpcs3_detected = true;
        m_requested_mode = ModePs3;
        m_mode_from_profile = true;
    }
    
    bool has_seen_assignment(uint32_t mask) const { return m_seen_masks & mask; }
    void mark_seen_assignment(uint32_t mask) { m_seen_masks |= mask; }
    void clear_seen_masks()
    {
        m_seen_masks = 0;
        m_local_inputs_owner = nullptr;
    }

    // The onboard inputs go to the first profile that claims them each load, so only one of the
    // profiles that output them (eg one for USB and one for bluetooth) is active at a time
    bool local_inputs_available_to(const Profile *profile) const { return !m_local_inputs_owner || m_local_inputs_owner == profile; }
    void claim_local_inputs(const Profile *profile) { m_local_inputs_owner = profile; }

    // Startup triggers that matched, so the profile they picked survives later reloads (eg once
    // the console is detected), long after the input was let go. Kept until a new config is written.
    bool is_startup_trigger_latched(const StartupTriggerKey &key) const;
    void latch_startup_trigger(const StartupTriggerKey &key);
    void clear_startup_triggers() { m_latched_startup_triggers.clear(); }
    
    // Clear all state
    void clear_all();

private:
    ConfigManager() 
        : m_current_mode(ModeHid)
        , m_requested_mode(ModeHid)
        , m_mode_changed(false)
        , m_working(false)
        , m_loaded_any(false)
        , m_has_bluetooth(false)
        , m_reloading(false)
        , m_full_reload(false)
        , m_reinitialize_device_stack(false)
        , m_reinit_time(0)
        , m_time_since_mode(0)
        , m_seen_masks(0)
    {}

    char m_device_name[sizeof(proto_Config::deviceName)] = {};
    
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    ConsoleMode m_current_mode;
    ConsoleMode m_requested_mode;
    bool m_mode_changed;
    bool m_working;
    bool m_loaded_any;
    bool m_has_bluetooth;
    bool m_reloading;
    bool m_full_reload;
    bool m_reinitialize_device_stack;
    bool m_restart_device_stack = false;
    bool m_forced_mode_deferred = false;
    bool m_rpcs3_detected = false;
    ConsoleMode m_deferred_mode = ModeHid;
    bool m_mode_from_profile = false;
    ConsoleMode m_detected_mode = ModeHid;
    uint32_t m_last_work = 0;
    uint32_t m_reinit_time;
    uint32_t m_time_since_mode;
    uint32_t m_seen_masks;
    const Profile *m_local_inputs_owner = nullptr;
    std::vector<StartupTriggerKey> m_latched_startup_triggers;
};
