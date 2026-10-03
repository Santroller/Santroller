#pragma once

#include "input.pb.h"
#include "protocols/switch_arcade.hpp"

inline void switch_arcade_update_button(proto_Output const &mapping, bool pressed,
                                        SwitchArcadeReport &report)
{
    if (!pressed || mapping.which_mapping != proto_Output_gamepadButton_tag)
        return;

    uint16_t button = 0;
    switch (mapping.mapping.gamepadButton)
    {
    case Gamepad_Y:               button = SwitchArcade_Y; break;
    case Gamepad_B:               button = SwitchArcade_B; break;
    case Gamepad_A:               button = SwitchArcade_A; break;
    case Gamepad_X:               button = SwitchArcade_X; break;
    case Gamepad_LeftShoulder:    button = SwitchArcade_L; break;
    case Gamepad_RightShoulder:   button = SwitchArcade_R; break;
    case Gamepad_Back:            button = SwitchArcade_Minus; break;
    case Gamepad_Start:           button = SwitchArcade_Plus; break;
    case Gamepad_LeftThumbClick:  button = SwitchArcade_LS; break;
    case Gamepad_RightThumbClick: button = SwitchArcade_RS; break;
    case Gamepad_Guide:           button = SwitchArcade_Home; break;
    case Gamepad_Capture:         button = SwitchArcade_Capture; break;
    case Gamepad_DpadUp:          report.hat |= 0x10; break;
    case Gamepad_DpadDown:        report.hat |= 0x20; break;
    case Gamepad_DpadLeft:        report.hat |= 0x40; break;
    case Gamepad_DpadRight:       report.hat |= 0x80; break;
    default: break;
    }
    switch_arcade_set_button(report, button);
}

inline void switch_arcade_update_axis(proto_Output const &mapping, uint32_t value,
                                      bool centered, SwitchArcadeReport &report)
{
    if (centered || mapping.which_mapping != proto_Output_gamepadAxis_tag)
        return;

    switch (mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:   report.lx = value >> 8; break;
    case Gamepad_LeftStickY:   report.ly = (UINT16_MAX - value) >> 8; break;
    case Gamepad_RightStickX:  report.rx = value >> 8; break;
    case Gamepad_RightStickY:  report.ry = (UINT16_MAX - value) >> 8; break;
    case Gamepad_LeftTrigger:
        if (value > 60000) switch_arcade_set_button(report, SwitchArcade_ZL);
        break;
    case Gamepad_RightTrigger:
        if (value > 60000) switch_arcade_set_button(report, SwitchArcade_ZR);
        break;
    default: break;
    }
}
