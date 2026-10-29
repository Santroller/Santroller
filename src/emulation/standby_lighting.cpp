#include "emulation/standby_lighting.hpp"
#include "emulation/usb/device.hpp"
#include "leds/led_mappings.hpp"
#include "triggers/input_trigger.hpp"
#include "device/dcd.h"

void StandbyLightingInstance::update_leds(bool full_poll, bool send_events)
{
    for (const auto &profile : profiles)
    {
        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
    }
}

void StandbyLightingInstance::leds_off()
{
    for (const auto &profile : profiles)
    {
        for (const auto &led : profile->leds)
        {
            led->off();
        }
    }
}

bool StandbyLightingInstance::lights_while_suspended() const
{
    for (const auto &profile : profiles)
    {
        if (profile->standby_while_suspended)
        {
            return true;
        }
    }
    return false;
}

bool StandbyLightingInstance::has_wake_buttons() const
{
    for (const auto &profile : profiles)
    {
        if (!profile->standby_wake.empty())
        {
            return true;
        }
    }
    return false;
}

bool StandbyLightingInstance::wake_pressed() const
{
    bool pressed = false;
    for (const auto &profile : profiles)
    {
        for (const auto &wake : profile->standby_wake)
        {
            pressed |= wake->calc_val();
        }
    }
    return pressed;
}

void StandbyLightingInstance::process(bool full_poll, bool send_events)
{
    update_leds(full_poll, send_events);
    bool pressed = wake_pressed();
    if (!pressed)
    {
        m_wake_armed = true;
        return;
    }
    if (!m_wake_armed)
    {
        return;
    }
    m_wake_armed = false;
    // A powered off 360 never resets the bus, it only idles it. Like a real controller, signalling resume
    // turns it on, even though the host never configured us (so remote wakeup was never enabled). Once it
    // has, the USB profile is the one that wakes it.
    if (UsbDevice::mounted_since_init || !UsbDevice::bus_idle())
    {
        return;
    }
    printf("standby wake\r\n");
    dcd_remote_wakeup(TUD_OPT_RHPORT);
}
