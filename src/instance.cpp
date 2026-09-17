#include "instance.hpp"
#include "profiles/profile.hpp"
#include "devices/base.hpp"
#include "enums.pb.h"

void Instance::set_rumble(uint8_t left, uint8_t right)
{
    if (rumble_left == left && rumble_right == right)
    {
        return;
    }
    rumble_left = left;
    rumble_right = right;
    update_feedback();
}

void Instance::set_player_led(uint8_t player)
{
    if (player_led == player)
    {
        return;
    }
    player_led = player;
    update_feedback();
}

void Instance::set_lightbar(uint8_t r, uint8_t g, uint8_t b)
{
    if (lightbar_red == r && lightbar_green == g && lightbar_blue == b)
    {
        return;
    }
    lightbar_red = r;
    lightbar_green = g;
    lightbar_blue = b;
    update_feedback();
}

void Instance::set_euphoria_led(uint8_t val)
{
    if (euphoria_led == val)
    {
        return;
    }
    euphoria_led = val;
    update_feedback();
}

void Instance::update_feedback(bool force)
{
    for (const auto &profile : profiles)
    {
        if (!profile)
        {
            continue;
        }
        for (const auto &dev_pair : profile->devices)
        {
            if (dev_pair.second)
            {
                dev_pair.second->set_rumble(rumble_left, rumble_right);
                dev_pair.second->set_player_led(player_led);
                dev_pair.second->set_lightbar(lightbar_red, lightbar_green, lightbar_blue);
                dev_pair.second->set_euphoria_led(euphoria_led > 0);
            }
        }
    }
}

void Instance::update_capabilities()
{
    uint8_t caps = 0;
    for (const auto &profile : profiles)
    {
        if (!profile)
        {
            continue;
        }
        for (const auto &dev_pair : profile->devices)
        {
            if (dev_pair.second)
            {
                if (dev_pair.second->has_rumble())
                {
                    caps |= CapabilityHasRumble;
                }
                if (dev_pair.second->has_player_led())
                {
                    caps |= CapabilityHasStandardPlayerLeds;
                }
                if (dev_pair.second->has_euphoria_led())
                {
                    caps |= CapabilityHasRGBIndicatorLed;
                }
            }
        }
        if (!profile->leds.empty())
        {
            caps |= CapabilityHasStandardPlayerLeds;
        }
    }
    if (subtype == Gamepad)
    {
        caps |= CapabilityHasRumble;
    }
    else if (subtype == DjHeroTurntable)
    {
        caps |= CapabilityHasRGBIndicatorLed;
    }
    else if (subtype == StageKit)
    {
        caps |= CapabilityHasInstrumentLeds;
    }
    capabilities = caps;
}

