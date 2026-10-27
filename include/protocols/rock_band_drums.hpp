#pragma once
#include <stdint.h>
#include "input_enums.pb.h"

// Rock Band kits report hits the same way on every console: a face button per colour, a pad and a
// cymbal flag, and the d-pad to say which colour a cymbal hit was. Only the report layout and the
// velocity encoding differ, so hosts pull these out of their report and decode them here.

struct RockBandDrumHits
{
    uint16_t red_pad;
    uint16_t yellow_pad;
    uint16_t blue_pad;
    uint16_t green_pad;
    uint16_t yellow_cymbal;
    uint16_t blue_cymbal;
    uint16_t green_cymbal;
};

struct RockBandDrumReport
{
    bool green, red, yellow, blue;
    bool pad, cymbal;
    bool up, down;
    // Whether the raw red velocity is set at all, as a pad and cymbal of the same colour hit
    // together put the cymbal velocity in red without red being pressed
    bool red_velocity_set;
    // Velocities scaled to 0 - 65535, only meaningful while the matching colour is hit
    uint16_t red_velocity, yellow_velocity, blue_velocity, green_velocity;
};

// RB1 kits have no velocity or cymbals and report every hit as a plain face button. RB2+ kits
// flag pads and cymbals, and without a flag the face buttons are just navigation.
inline RockBandDrumHits rb_decode_drum_hits(const RockBandDrumReport &report, bool rb2)
{
    RockBandDrumHits hits = {};
    if (!rb2)
    {
        hits.red_pad = report.red ? UINT16_MAX : 0;
        hits.yellow_pad = report.yellow ? UINT16_MAX : 0;
        hits.blue_pad = report.blue ? UINT16_MAX : 0;
        hits.green_pad = report.green ? UINT16_MAX : 0;
        return hits;
    }
    bool green = report.green;
    bool red = report.red;
    bool yellow = report.yellow;
    bool blue = report.blue;
    bool pad = report.pad;
    bool cymbal = report.cymbal;
    bool up = report.up;
    bool down = report.down;
    if (pad && cymbal)
    {
        // Only the d-pad is checked to resolve a pad+cymbal hit, as the colour flag can be
        // released before the d-pad is (particularly at high polling rates)
        if (!red && report.red_velocity_set)
        {
            // Pad and cymbal of the same colour hit together: the cymbal velocity goes in
            // red, without red being pressed
            if (up)
            {
                hits.yellow_cymbal = report.red_velocity;
                hits.yellow_pad = report.yellow_velocity;
            }
            else if (down)
            {
                hits.blue_cymbal = report.red_velocity;
                hits.blue_pad = report.blue_velocity;
            }
            else
            {
                hits.green_cymbal = report.red_velocity;
                hits.green_pad = report.green_velocity;
            }
            return hits;
        }
        if (up)
        {
            hits.yellow_cymbal = report.yellow_velocity;
            yellow = false;
        }
        if (down)
        {
            hits.blue_cymbal = report.blue_velocity;
            blue = false;
        }
        if (!(up || down))
        {
            hits.green_cymbal = report.green_velocity;
            green = false;
        }
        cymbal = false;
    }
    if (pad)
    {
        if (red)
            hits.red_pad = report.red_velocity;
        if (yellow)
            hits.yellow_pad = report.yellow_velocity;
        if (blue)
            hits.blue_pad = report.blue_velocity;
        if (green)
            hits.green_pad = report.green_velocity;
    }
    if (cymbal)
    {
        if (yellow)
            hits.yellow_cymbal = report.yellow_velocity;
        if (blue)
            hits.blue_cymbal = report.blue_velocity;
        if (green)
            hits.green_cymbal = report.green_velocity;
    }
    return hits;
}

inline uint16_t rb_drum_hit(const RockBandDrumHits &hits, RockBandDrumsAxisType axis)
{
    switch (axis)
    {
    case RockBandDrums_RedPad:
        return hits.red_pad;
    case RockBandDrums_YellowPad:
        return hits.yellow_pad;
    case RockBandDrums_BluePad:
        return hits.blue_pad;
    case RockBandDrums_GreenPad:
        return hits.green_pad;
    case RockBandDrums_YellowCymbal:
        return hits.yellow_cymbal;
    case RockBandDrums_BlueCymbal:
        return hits.blue_cymbal;
    case RockBandDrums_GreenCymbal:
        return hits.green_cymbal;
    default:
        return 0;
    }
}

// RB4 kits (Xbox One, PS4 and PS5) press the matching face button on every hit as well, but give
// each pad and cymbal its own velocity, so a face button is only navigation while its colour has
// no velocity. Their reports all use the same field names, so this takes any of them.
template <typename Report>
inline bool rb4_drums_face_button(const Report *report, GamepadButtonType button)
{
    switch (button)
    {
    case Gamepad_A:
        return report->a && !report->greenVelocity && !report->greenCymbalVelocity;
    case Gamepad_B:
        return report->b && !report->redVelocity;
    case Gamepad_X:
        return report->x && !report->blueVelocity && !report->blueCymbalVelocity;
    case Gamepad_Y:
        return report->y && !report->yellowVelocity && !report->yellowCymbalVelocity;
    default:
        return false;
    }
}

template <typename Report>
inline uint8_t rb4_drums_velocity(const Report *report, RockBandDrumsAxisType axis)
{
    switch (axis)
    {
    case RockBandDrums_RedPad:
        return report->redVelocity;
    case RockBandDrums_YellowPad:
        return report->yellowVelocity;
    case RockBandDrums_BluePad:
        return report->blueVelocity;
    case RockBandDrums_GreenPad:
        return report->greenVelocity;
    case RockBandDrums_YellowCymbal:
        return report->yellowCymbalVelocity;
    case RockBandDrums_BlueCymbal:
        return report->blueCymbalVelocity;
    case RockBandDrums_GreenCymbal:
        return report->greenCymbalVelocity;
    default:
        return 0;
    }
}

// Stock Xbox One kits only use 0-7 of their 4 bit velocities, their hardest hits reaching around 5, so
// generated velocities are scaled to that range and anything above it from a hosted kit is clamped. Stock
// PS4 / PS5 kits use the whole byte
#define RB4_XBOX_ONE_MAX_VELOCITY 7
#define RB4_PS_MAX_VELOCITY 0xFF

inline uint16_t rb4_drums_velocity_to_axis(uint8_t velocity, uint8_t max)
{
    if (velocity > max)
    {
        velocity = max;
    }
    return velocity * UINT16_MAX / max;
}

// A light hit must never round to 0 or the game reads it as the other pad type
inline uint8_t rb4_drums_axis_to_velocity(uint16_t value, uint8_t max)
{
    uint8_t velocity = ((uint32_t)value * max + UINT16_MAX / 2) / UINT16_MAX;
    return velocity ? velocity : 1;
}
