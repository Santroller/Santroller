#include "managers/config_manager.hpp"

bool ConfigManager::mode_recently_changed(uint32_t now, uint32_t window_ms) const
{
    return (now - m_time_since_mode) < window_ms;
}

bool ConfigManager::is_startup_trigger_latched(const StartupTriggerKey &key) const
{
    for (const auto &latched : m_latched_startup_triggers)
    {
        if (latched == key)
            return true;
    }
    return false;
}

void ConfigManager::latch_startup_trigger(const StartupTriggerKey &key)
{
    if (!is_startup_trigger_latched(key))
        m_latched_startup_triggers.push_back(key);
}

bool ConfigManager::should_reinit(uint32_t now) const
{
    return m_requested_mode != m_current_mode || (m_reinit_time && now > m_reinit_time);
}

void ConfigManager::begin_reinit()
{
    m_mode_changed = true;
    m_reloading = true;
    m_reinit_time = 0;
}

void ConfigManager::finish_reinit(uint32_t now)
{
    m_time_since_mode = now;
    m_reloading = false;
    m_full_reload = false;
    m_reinitialize_device_stack = false;
}

void ConfigManager::clear_all()
{
    m_current_mode = ModeHid;
    m_requested_mode = ModeHid;
    m_mode_changed = false;
    m_working = false;
    m_loaded_any = false;
    m_has_bluetooth = false;
    m_reloading = false;
    m_reinitialize_device_stack = false;
    m_restart_device_stack = false;
    m_reinit_time = 0;
    m_time_since_mode = 0;
    m_seen_masks = 0;
    m_forced_mode_deferred = false;
    m_latched_startup_triggers.clear();
}
