// SOCD cleaning, separate from profile.cpp so the mapping tests can link it without the rest of the profile
#include "profiles/profile.hpp"
#include "mappings/base_mapping.hpp"

uint8_t Profile::socd_bit_for_output(const proto_Output &output) const
{
    if (socd_mode == SocdOff || (subtype != Gamepad && subtype != FightStick) ||
        output.which_mapping != proto_Output_gamepadButton_tag)
    {
        return 0;
    }
    switch (output.mapping.gamepadButton)
    {
    case Gamepad_DpadUp:
        return SOCD_UP;
    case Gamepad_DpadDown:
        return SOCD_DOWN;
    case Gamepad_DpadLeft:
        return SOCD_LEFT;
    case Gamepad_DpadRight:
        return SOCD_RIGHT;
    default:
        return 0;
    }
}

void Profile::update_socd(bool full_poll, bool send_events)
{
    uint8_t held = 0;
    for (auto *mapping : socd_mappings)
    {
        mapping->update_value(full_poll, send_events);
        if (mapping->value())
        {
            held |= mapping->socd_bit();
        }
    }
    uint8_t pressed = held & ~socd_held;
    socd_held = held;
    uint8_t allowed = held;
    for (int axis = 0; axis < 2; axis++)
    {
        uint8_t pair = axis ? (SOCD_LEFT | SOCD_RIGHT) : (SOCD_UP | SOCD_DOWN);
        if (pressed & pair)
        {
            socd_newest[axis] = (pressed & pair) == pair ? 0 : pressed & pair;
        }
        if ((held & pair) != pair)
        {
            continue;
        }
        // Neutral, and anything pressed at the same time, keeps neither direction
        uint8_t keep = 0;
        switch (socd_mode)
        {
        case SocdUpPriority:
            keep = axis ? 0 : SOCD_UP;
            break;
        case SocdLastInput:
            keep = socd_newest[axis];
            break;
        case SocdFirstInput:
            keep = socd_newest[axis] ? pair & ~socd_newest[axis] : 0;
            break;
        default:
            break;
        }
        allowed = (allowed & ~pair) | keep;
    }
    for (auto *mapping : socd_mappings)
    {
        mapping->mask_socd(allowed);
    }
}
