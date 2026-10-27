#include "emulation/standby_lighting.hpp"
#include "leds/led_mappings.hpp"

void StandbyLightingInstance::process(bool full_poll, bool send_events)
{
    for (const auto &profile : profiles)
    {
        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
    }
}
