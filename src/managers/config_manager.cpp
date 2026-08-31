#include "managers/config_manager.hpp"

bool ConfigManager::mode_recently_changed(uint32_t now, uint32_t window_ms) const
{
    return (now - m_time_since_mode) < window_ms;
}

bool ConfigManager::should_reinit(uint32_t now) const
{
    return m_new_mode != m_mode || (m_reinit_time && now > m_reinit_time);
}

void ConfigManager::begin_reinit()
{
    m_mode_changed = true;
    m_reloading = true;
    m_mode = m_new_mode;
    m_reinit_time = 0;
}

void ConfigManager::finish_reinit(uint32_t now)
{
    m_time_since_mode = now;
    m_reloading = false;
}

void ConfigManager::clear_all()
{
    m_mode = ModeHid;
    m_new_mode = ModeHid;
    m_mode_changed = false;
    m_working = false;
    m_loaded_any = false;
    m_has_bluetooth = false;
    m_reloading = false;
    m_reinit_time = 0;
    m_time_since_mode = 0;
    m_seen_masks = 0;
}
