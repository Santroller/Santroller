#pragma once
#include "instance.hpp"

// The standby lighting profile, while no other profile is active. Nothing is output, it only
// runs the profile's LEDs
class StandbyLightingInstance : public Instance
{
public:
    void initialize() {}
    void process(bool full_poll, bool send_events);
};
