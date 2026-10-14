#pragma once
#include <stdint.h>
#include "protocols/ps3.hpp"
#include "protocols/rock_band_drums.hpp"

// Decoding PS3 (and Wii USB) drum kits from the report they send. The structs in ps3.hpp are laid
// out for building reports, with d-pad bitfields in byte 2, but kits send a hat switch there.

// Hat switch: 0 is up, going clockwise to 7 (up left), anything else is centred
inline bool ps3_hat_up(const uint8_t *buf)
{
    uint8_t hat = buf[2] & 0x0F;
    return hat == 7 || hat == 0 || hat == 1;
}

inline bool ps3_hat_down(const uint8_t *buf)
{
    uint8_t hat = buf[2] & 0x0F;
    return hat >= 3 && hat <= 5;
}

// RB velocities are inverted so RB1 kits, which always send 0, read as the hardest hit
inline uint16_t ps3_rb_drum_velocity(uint8_t velocity)
{
    return (0xFF - velocity) * 0x101;
}

inline RockBandDrumHits ps3_decode_rb_drums(const uint8_t *buf, bool rb2)
{
    auto data = (const PS3RockBandDrums_Data_t *)buf;
    RockBandDrumReport report = {};
    report.green = data->a;
    report.red = data->b;
    report.yellow = data->y;
    report.blue = data->x;
    report.pad = data->padFlag;
    report.cymbal = data->cymbalFlag;
    report.up = ps3_hat_up(buf);
    report.down = ps3_hat_down(buf);
    report.red_velocity_set = data->redVelocity != 0;
    report.red_velocity = ps3_rb_drum_velocity(data->redVelocity);
    report.yellow_velocity = ps3_rb_drum_velocity(data->yellowVelocity);
    report.blue_velocity = ps3_rb_drum_velocity(data->blueVelocity);
    report.green_velocity = ps3_rb_drum_velocity(data->greenVelocity);
    return rb_decode_drum_hits(report, rb2);
}

// RB2+ kits press the face buttons and d-pad when hitting pads and cymbals, so they only count as
// navigation when no hit is flagged
inline bool ps3_rb_drums_hit_flagged(const uint8_t *buf)
{
    auto data = (const PS3RockBandDrums_Data_t *)buf;
    return data->padFlag || data->cymbalFlag;
}

inline bool ps3_rb_drum_button(const uint8_t *buf, RockBandDrumsButtonType button)
{
    auto data = (const PS3RockBandDrums_Data_t *)buf;
    switch (button)
    {
    case RockBandDrums_Kick1Pedal:
        return data->kick1;
    case RockBandDrums_Kick2Pedal:
        return data->kick2;
    default:
        return false;
    }
}

// Guitar Hero kits report standard 7 bit MIDI velocities
inline uint16_t ps3_gh_drum_axis(const uint8_t *buf, GuitarHeroDrumsAxisType axis)
{
    auto data = (const PS3GuitarHeroDrums_Data_t *)buf;
    switch (axis)
    {
    case GuitarHeroDrums_RedPad:
        return (uint16_t)(data->redVelocity << 9);
    case GuitarHeroDrums_YellowPad:
        return (uint16_t)(data->yellowVelocity << 9);
    case GuitarHeroDrums_BluePad:
        return (uint16_t)(data->blueVelocity << 9);
    case GuitarHeroDrums_OrangePad:
        return (uint16_t)(data->orangeVelocity << 9);
    case GuitarHeroDrums_GreenPad:
        return (uint16_t)(data->greenVelocity << 9);
    case GuitarHeroDrums_KickPedal:
        return (uint16_t)(data->kickVelocity << 9);
    default:
        return 0;
    }
}
