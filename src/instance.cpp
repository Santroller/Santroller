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
    case RUMBLE_STAGEKIT_FOG_ON:
        stagekit_fog = 0xff;
        break;
    case RUMBLE_STAGEKIT_FOG_OFF:
        stagekit_fog = 0x00;
        break;
    // ms between flashes, the same as santroller v1
    case RUMBLE_STAGEKIT_SLOW_STROBE:
        stagekit_strobe_speed = 150;
        stagekit_last_strobe = millis();
        break;
    case RUMBLE_STAGEKIT_MEDIUM_STROBE:
        stagekit_strobe_speed = 125;
        stagekit_last_strobe = millis();
        break;
    case RUMBLE_STAGEKIT_FAST_STROBE:
        stagekit_strobe_speed = 100;
        stagekit_last_strobe = millis();
        break;
    case RUMBLE_STAGEKIT_FASTEST_STROBE:
        stagekit_strobe_speed = 75;
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
        stagekit_blue = param;
        break;
    case RUMBLE_STAGEKIT_GREEN:
        stagekit_green = param;
        break;
    case RUMBLE_STAGEKIT_YELLOW:
        stagekit_yellow = param;
        break;
    case RUMBLE_STAGEKIT_RED:
        stagekit_red = param;
        break;
    case RUMBLE_SANTROLLER_NOTE_MISS:
        game_feedback.note_miss = param;
        break;
    case RUMBLE_SANTROLLER_MULTIPLIER:
        game_feedback.multiplier = param;
        break;
    case RUMBLE_SANTROLLER_SOLO:
        game_feedback.solo = param;
        break;
    case RUMBLE_SANTROLLER_STAR_POWER_ACTIVE:
        game_feedback.star_power_active = param;
        break;
    case RUMBLE_SANTROLLER_STAR_POWER_FILL:
        game_feedback.star_power_fill = param;
        break;
    case RUMBLE_SANTROLLER_NOTE_HIT:
        game_feedback.note_hits = param;
        break;
    case RUMBLE_SANTROLLER_EUPHORIA_LED:
        euphoria_led = param;
        break;
    }
    update_feedback();
}

void Instance::process_feedback_report(const uint8_t *data, uint16_t len)
{
    // type, strobe:7 fog:1, blue, green, yellow, red, multiplier, star power fill,
    // star power active:1 solo:1 miss:1, then the device specific note hits
    if (len < 9)
    {
        return;
    }
    // Strobe goes 1 (slow) to 4 (fastest), and 5 for off. ms between flashes match the
    // single stage kit commands.
    static const uint16_t strobe_speeds[] = {0, 150, 125, 100, 75};
    uint8_t strobe = data[1] & 0x7F;
    uint16_t strobe_speed = strobe < 5 ? strobe_speeds[strobe] : 0;
    if (strobe_speed != stagekit_strobe_speed)
    {
        stagekit_strobe_speed = strobe_speed;
        stagekit_last_strobe = strobe_speed ? millis() : 0;
    }
    stagekit_fog = data[1] & 0x80 ? 0xFF : 0;
    stagekit_blue = data[2];
    stagekit_green = data[3];
    stagekit_yellow = data[4];
    stagekit_red = data[5];
    game_feedback.multiplier = data[6];
    game_feedback.star_power_fill = data[7];
    game_feedback.star_power_active = data[8] & 0x01;
    game_feedback.solo = data[8] & 0x02;
    game_feedback.note_miss = data[8] & 0x04;
    game_feedback.note_hits = len > 9 ? data[9] : 0;
    update_feedback();
}

void Instance::set_keyboard_leds(uint8_t leds)
{
    if (keyboard_leds == leds)
    {
        return;
    }
    keyboard_leds = leds;
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
        for (const auto &dev_pair : profile->claimed_devices)
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
                led_pair->set_game_feedback(game_feedback);
                led_pair->set_keyboard_leds(keyboard_leds);
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
        for (const auto &dev_pair : profile->claimed_devices)
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
