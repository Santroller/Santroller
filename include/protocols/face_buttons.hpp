#pragma once
#include "input.pb.h"

// Gamepad_A / B / X / Y name the legends on the buttons in legend based mode. In position based mode they
// name where the buttons sit instead, as on an Xbox pad: A south, B east, X west and Y north.
//
// Nintendo pads swap both pairs (A east, B south, X north, Y west), so going between an Xbox position and
// a Nintendo legend is the same swap in both directions.
static inline GamepadButtonType swap_nintendo_face_button(GamepadButtonType button)
{
    switch (button)
    {
    case Gamepad_A: return Gamepad_B;
    case Gamepad_B: return Gamepad_A;
    case Gamepad_X: return Gamepad_Y;
    case Gamepad_Y: return Gamepad_X;
    default:        return button;
    }
}

static inline proto_Output swap_nintendo_face_button(proto_Output output)
{
    if (output.which_mapping == proto_Output_gamepadButton_tag)
    {
        output.mapping.gamepadButton = swap_nintendo_face_button(output.mapping.gamepadButton);
    }
    return output;
}

// The GameCube has A in the middle, B to its left, X to its right and Y on top
static inline GamepadButtonType gamecube_button_at(GamepadButtonType position)
{
    switch (position)
    {
    case Gamepad_B: return Gamepad_X;
    case Gamepad_X: return Gamepad_B;
    default:        return position;
    }
}
