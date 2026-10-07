#pragma once
#include <stdint.h>

// The controller's battery level, from whichever battery device is configured
class BatteryManager
{
public:
    static BatteryManager &instance()
    {
        static BatteryManager manager;
        return manager;
    }
    void set_level(uint8_t percent)
    {
        m_level = percent > 100 ? 100 : percent;
        m_has_level = true;
    }
    bool has_level() const { return m_has_level; }
    // 100 when nothing reports a level, so hosts don't warn about a battery that isn't there
    uint8_t level() const { return m_has_level ? m_level : 100; }

    // Whether the config has a battery device. Only then does the HID descriptor include a
    // battery, so controllers without one keep the descriptor consoles already know.
    void begin_config_reload()
    {
        m_previous_present = m_present;
        m_present = false;
    }
    void set_present() { m_present = true; }
    bool present() const { return m_present; }
    bool present_changed() const { return m_present != m_previous_present; }

    // Battery state in the forms console reports use
    // DualShock 4: level 0 to 10 in the low nibble, and bit 4 for the cable being plugged in
    uint8_t ps4_status() const { return 0x10 | ((level() + 5) / 10); }
    // Switch: 0 (empty) to 8 (full) in steps of 2
    uint8_t switch_level() const { return (level() + 12) / 25 * 2; }
    // DualShock 3 on USB only reports charging (0xEE) or charged (0xEF)
    uint8_t ps3_status() const { return level() < 100 ? 0xEE : 0xEF; }

private:
    BatteryManager() = default;
    uint8_t m_level = 100;
    bool m_has_level = false;
    bool m_present = false;
    bool m_previous_present = false;
};
