#include "emulation/wii_extension_input.hpp"
#include "mappings/base_mapping.hpp"
#include "leds/led_mappings.hpp"
#include "protocols/wii.hpp"
#include <stddef.h>
#include <string.h>

uint8_t wii_extension_format_for_subtype(SubType subtype, uint8_t detected_format)
{
    return subtype == Gamepad ? detected_format : 3;
}

uint8_t wii_extension_report_size(SubType subtype, uint8_t format)
{
    if (subtype == Taiko)
        return sizeof(WiiTaikoData_t);
    if (subtype == GuitarHeroGuitar)
    {
        return sizeof(WiiGuitarDataFormat3_t);
    }
    if (subtype == GuitarHeroDrums)
    {
        return sizeof(WiiDrumDataFormat3_t);
    }
    if (subtype == DjHeroTurntable)
    {
        return sizeof(WiiTurntableDataFormat3_t);
    }
    if (format == 1)
    {
        return sizeof(WiiClassicDataFormat1_t);
    }
    if (format == 2)
    {
        return sizeof(WiiClassicDataFormat2_t);
    }
    return sizeof(WiiClassicDataFormat3_t);
}

void initialize_wii_extension_report(SubType subtype, uint8_t format,
                                      uint8_t *report, uint8_t *size,
                                      uint8_t *buttons_low_offset,
                                      uint8_t *buttons_high_offset)
{
    memset(report, 0, wii_extension_report_size(subtype, format));
    if (subtype == Taiko)
    {
        auto *data = reinterpret_cast<WiiTaikoData_t *>(report);
        data->staticData[0] = 0xA0;
        data->staticData[1] = 0x20;
        data->staticData[2] = 0x50;
        data->staticData[3] = 0x10;
        data->staticData[4] = 0xFF;
        if (buttons_low_offset) *buttons_low_offset = offsetof(WiiTaikoData_t, buttons);
        if (buttons_high_offset) *buttons_high_offset = offsetof(WiiTaikoData_t, buttons);
    }
    else if (subtype == GuitarHeroGuitar)
    {
        auto *data = reinterpret_cast<WiiGuitarDataFormat3_t *>(report);
        data->leftStickX = 32;
        data->leftStickY = 32;
        // 0x0F is the touch bar's "not touching" code (wiibrew Wiimote/Extension_Controllers/Guitar_Hero_(Wii)_Guitars)
        data->slider = 0x0F;
        if (buttons_low_offset) *buttons_low_offset = offsetof(WiiGuitarDataFormat3_t, buttonsLow);
        if (buttons_high_offset) *buttons_high_offset = offsetof(WiiGuitarDataFormat3_t, buttonsHigh);
    }
    else if (subtype == GuitarHeroDrums)
    {
        auto *data = reinterpret_cast<WiiDrumDataFormat3_t *>(report);
        data->leftStickX = 32;
        data->leftStickY = 32;
        // the drum data is inverted, so no hit is FF FF in bytes 2 and 3 (wiibrew Wiimote/Extension_Controllers/Guitar_Hero_World_Tour_(Wii)_Drums)
        report[2] = 0xFF;
        report[3] = 0xFF;
        if (buttons_low_offset) *buttons_low_offset = offsetof(WiiDrumDataFormat3_t, buttonsLow);
        if (buttons_high_offset) *buttons_high_offset = offsetof(WiiDrumDataFormat3_t, buttonsHigh);
    }
    else if (subtype == DjHeroTurntable)
    {
        auto *data = reinterpret_cast<WiiTurntableDataFormat3_t *>(report);
        data->leftStickX = 32;
        data->leftStickY = 32;
        if (buttons_low_offset) *buttons_low_offset = offsetof(WiiTurntableDataFormat3_t, buttonsLow);
        if (buttons_high_offset) *buttons_high_offset = offsetof(WiiTurntableDataFormat3_t, buttonsHigh);
    }
    else if (format == 1)
    {
        WiiIntermediateClassicDataFormat_t intermediate = {};
        auto *data = reinterpret_cast<WiiClassicDataFormat1_t *>(report);
        intermediate.rightStickX = 16;
        data->leftStickX = 32;
        data->leftStickY = 32;
        data->rightStickY = 16;
        data->rightStickX0 = intermediate.rightStickX0;
        data->rightStickX21 = intermediate.rightStickX21;
        data->rightStickX43 = intermediate.rightStickX43;
        if (buttons_low_offset) *buttons_low_offset = offsetof(WiiClassicDataFormat1_t, buttonsLow);
        if (buttons_high_offset) *buttons_high_offset = offsetof(WiiClassicDataFormat1_t, buttonsHigh);
    }
    else if (format == 2)
    {
        auto *data = reinterpret_cast<WiiClassicDataFormat2_t *>(report);
        data->leftStickX92 = 128;
        data->leftStickY92 = 128;
        data->rightStickX92 = 128;
        data->rightStickY92 = 128;
        if (buttons_low_offset) *buttons_low_offset = offsetof(WiiClassicDataFormat2_t, buttonsLow);
        if (buttons_high_offset) *buttons_high_offset = offsetof(WiiClassicDataFormat2_t, buttonsHigh);
    }
    else
    {
        auto *data = reinterpret_cast<WiiClassicDataFormat3_t *>(report);
        data->leftStickX = 128;
        data->leftStickY = 128;
        data->rightStickX = 128;
        data->rightStickY = 128;
        if (buttons_low_offset) *buttons_low_offset = offsetof(WiiClassicDataFormat3_t, buttonsLow);
        if (buttons_high_offset) *buttons_high_offset = offsetof(WiiClassicDataFormat3_t, buttonsHigh);
    }
    *size = wii_extension_report_size(subtype, format);
}

void finalize_wii_extension_report(SubType subtype, uint8_t *report,
                                   uint8_t buttons_low_offset,
                                   uint8_t buttons_high_offset)
{
    if (buttons_low_offset == buttons_high_offset)
    {
        report[buttons_low_offset] = ~report[buttons_low_offset];
        return;
    }
    // the buttons are active low, but the turntable's LTT<5> shares buttonsLow and is a plain
    // signed bit (wiibrew Wiimote/Extension_Controllers/DJ_Hero_(Wii)_Turntable)
    uint8_t not_inverted = 0;
    if (subtype == DjHeroTurntable)
    {
        WiiTurntableDataFormat3_t mask = {};
        mask.leftTableVelocity5 = 1;
        not_inverted = mask.buttonsLow;
    }
    report[buttons_low_offset] ^= (uint8_t)~not_inverted;
    report[buttons_high_offset] = ~report[buttons_high_offset];
}

void update_wii_extension_input(const std::vector<std::shared_ptr<Profile>> &profiles,
                                bool full_poll, bool send_events,
                                uint8_t format, uint8_t *report)
{
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            mapping->update_wii(format, report);
        }

        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
    }
}
