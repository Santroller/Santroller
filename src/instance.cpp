#include "instance.hpp"
#include "profiles/profile.hpp"
#include "devices/base.hpp"
#include "enums.pb.h"
#include "utils.h"
#include "emulation/usb/usb_descriptors.h"

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

void Instance::update_stagekit()
{
    if (stagekit_strobe_speed > 0 && (millis() - stagekit_last_strobe) >= stagekit_strobe_speed)
    {
        stagekit_strobe = 0xFF;
        stagekit_last_strobe = millis();
    }
    else if (stagekit_strobe_speed > 0 && (millis() - stagekit_last_strobe) > 10)
    {
        stagekit_strobe = 0;
    }
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

void Instance::process_stagekit_command(uint8_t command, uint8_t param)
{
    stagekit_param = param;
    stagekit_command = command;
    switch (command)
    {
        switch (rumble_right)
        {
        case RUMBLE_STAGEKIT_FOG_ON:
            stagekit_fog = 0xff;
            break;
        case RUMBLE_STAGEKIT_FOG_OFF:
            stagekit_fog = 0x00;
            break;
        case RUMBLE_STAGEKIT_SLOW_STROBE:
            stagekit_strobe_speed = 150;
            stagekit_last_strobe = millis();
            break;
        case RUMBLE_STAGEKIT_MEDIUM_STROBE:
            stagekit_strobe_speed = 200;
            stagekit_last_strobe = millis();
            break;
        case RUMBLE_STAGEKIT_FAST_STROBE:
            stagekit_strobe_speed = 300;
            stagekit_last_strobe = millis();
            break;
        case RUMBLE_STAGEKIT_FASTEST_STROBE:
            stagekit_strobe_speed = 400;
            stagekit_last_strobe = millis();
            break;
        case RUMBLE_STAGEKIT_NO_STROBE:
            stagekit_strobe_speed = 0;
            stagekit_last_strobe = 0;
            break;
        case RUMBLE_STAGEKIT_ALLOFF:
            stagekit_fog = 0x00;
            stagekit_strobe_speed = 0;
            stagekit_blue = 0x00;
            stagekit_green = 0x00;
            stagekit_yellow = 0x00;
            stagekit_red = 0x00;
            break;
        case RUMBLE_STAGEKIT_BLUE:
            stagekit_blue = rumble_left;
            break;
        case RUMBLE_STAGEKIT_GREEN:
            stagekit_green = rumble_left;
            break;
        case RUMBLE_STAGEKIT_YELLOW:
            stagekit_yellow = rumble_left;
            break;
        case RUMBLE_STAGEKIT_RED:
            stagekit_red = rumble_left;
            break;
            // TODO: deal with the extra santroller commands
            // case RUMBLE_SANTROLLER_NOTE_MISS:
            //     stage_kit_report.report.noteMiss = rumble_left;
            //     break;
            // case RUMBLE_SANTROLLER_MULTIPLIER:
            //     stage_kit_report.report.multiplier = rumble_left;
            //     break;
            // case RUMBLE_SANTROLLER_SOLO:
            //     stage_kit_report.report.soloActive = rumble_left;
            //     break;
            // case RUMBLE_SANTROLLER_STAR_POWER_ACTIVE:
            //     stage_kit_report.report.starPowerActive = rumble_left;
            //     break;
            // case RUMBLE_SANTROLLER_STAR_POWER_FILL:
            //     stage_kit_report.report.starPowerState = rumble_left;
            //     break;
            // case RUMBLE_SANTROLLER_NOTE_HIT:
            //     stage_kit_report.report.noteHitRaw = rumble_left;
            //     break;
            // case RUMBLE_SANTROLLER_EUPHORIA_LED:
            //     stage_kit_report.report.euphoriaBrightness = rumble_left;
            break;
        }
    }
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
                dev_pair.second->set_stagekit_led(stagekit_param, stagekit_command);
            }
        }
        for (const auto &led_pair : profile->leds)
        {
            if (led_pair)
            {
                led_pair->set_rumble(rumble_left, rumble_right);
                led_pair->set_player_led(player_led);
                led_pair->set_lightbar(lightbar_red, lightbar_green, lightbar_blue);
                led_pair->set_euphoria_led(euphoria_led);
                led_pair->set_stagekit_led(stagekit_fog, stagekit_strobe, stagekit_blue, stagekit_green, stagekit_yellow, stagekit_red);
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
