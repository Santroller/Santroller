#pragma once
#include <stdint.h>
#include "config.pb.h"

// Tracks how long it has been since the last input, for turning the LEDs off and
// putting the controller to sleep while it isn't being used
class InactivityManager
{
public:
    static InactivityManager& instance()
    {
        static InactivityManager manager;
        return manager;
    }

    void configure(const proto_InactivityConfig *config);
    void input_activity(uint32_t now) { m_last_activity = now; }
    uint32_t idle_ms(uint32_t now) const { return now - m_last_activity; }
    bool leds_off() const { return m_leds_off; }
    void tick();

private:
    InactivityManager() = default;
    ~InactivityManager() = default;
    InactivityManager(const InactivityManager&) = delete;
    InactivityManager& operator=(const InactivityManager&) = delete;

    void set_leds_off(bool off);
    void go_to_sleep();

    uint32_t m_last_activity = 0;
    uint32_t m_sleep_timeout_ms = 0;
    uint32_t m_led_timeout_ms = 0;
    int32_t m_wake_pin = -1;
    bool m_wake_active_high = false;
    bool m_leds_off = false;
};
