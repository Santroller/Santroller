#pragma once
#include <stdint.h>
#include "config.pb.h"

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
    void begin_config_load() { m_requested_mode = m_current_mode; }
    bool has_mode_changed() const { return m_mode_changed; }
    void set_mode_changed(bool changed) { m_mode_changed = changed; }
    bool mode_recently_changed(uint32_t now, uint32_t window_ms = 2000) const;
    void mark_mode_change_time(uint32_t now) { m_time_since_mode = now; }
    void sync_requested_mode_to_current() { m_requested_mode = m_current_mode; }
    
    // Config loading state
    bool is_working() const { return m_working; }
    void set_working(bool working) { m_working = working; }
    bool has_loaded_any() const { return m_loaded_any; }
    void set_loaded_any(bool loaded) { m_loaded_any = loaded; }
    bool has_bluetooth() const { return m_has_bluetooth; }
    void set_bluetooth_available(bool available) { m_has_bluetooth = available; }
    bool is_reloading() const { return m_reloading; }
    void set_reloading(bool reloading) { m_reloading = reloading; }
    bool should_reinit(uint32_t now) const;
    void begin_reinit();
    void finish_reinit(uint32_t now);
    uint32_t get_reinit_time() const { return m_reinit_time; }
    void schedule_reinit(uint32_t when) { m_reinit_time = when; }
    void clear_reinit() { m_reinit_time = 0; }
    
    bool has_seen_assignment(uint32_t mask) const { return m_seen_masks & mask; }
    void mark_seen_assignment(uint32_t mask) { m_seen_masks |= mask; }
    void clear_seen_masks() { m_seen_masks = 0; }
    
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
        , m_reinit_time(0)
        , m_time_since_mode(0)
        , m_seen_masks(0)
    {}
    
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
    uint32_t m_reinit_time;
    uint32_t m_time_since_mode;
    uint32_t m_seen_masks;
};
