#pragma once
#include "instance.hpp"

// The standby lighting profile. Nothing is output, it runs the profile's LEDs and wake buttons: while no other
// profile is active, or while all of them are asleep with their USB host
class StandbyLightingInstance : public Instance
{
public:
    void initialize() {}
    // While no other profile is active, which also wakes a powered off 360 on standbyWake
    void process(bool full_poll, bool send_events);
    void update_leds(bool full_poll, bool send_events);
    void leds_off();
    // standbyWhileSuspended
    bool lights_while_suspended() const;
    // With wake buttons, only they wake a host, rather than any of the active profiles' buttons
    bool has_wake_buttons() const;
    bool wake_pressed() const;

private:
    // Only wake on a fresh press, so a button held at power on doesn't, and one press only signals once
    bool m_wake_armed = false;
};
