#pragma once
#include "input.pb.h"

// Where a wheel sends each axis in a gamepad report (Microsoft's XInput wheel layout: steering on
// the left stick, the accelerator and brake on the triggers). The clutch and handbrake aren't part
// of it, so they take the spare stick axes.
inline GamepadAxisType wheel_gamepad_axis(WheelAxisType axis)
{
    switch (axis)
    {
    case Wheel_Accelerator:
        return Gamepad_RightTrigger;
    case Wheel_Brake:
        return Gamepad_LeftTrigger;
    case Wheel_Clutch:
        return Gamepad_LeftStickY;
    case Wheel_Handbrake:
        return Gamepad_RightStickY;
    case Wheel_Steering:
    default:
        return Gamepad_LeftStickX;
    }
}

// The wheel axis a gamepad axis carries on a wheel, if it carries one
inline bool gamepad_wheel_axis(GamepadAxisType axis, WheelAxisType &wheel)
{
    switch (axis)
    {
    case Gamepad_LeftStickX:
        wheel = Wheel_Steering;
        return true;
    case Gamepad_RightTrigger:
        wheel = Wheel_Accelerator;
        return true;
    case Gamepad_LeftTrigger:
        wheel = Wheel_Brake;
        return true;
    default:
        return false;
    }
}

// Rewrites a wheel axis as the gamepad axis it is sent on, leaving any other output alone
inline void wheel_axis_as_gamepad(proto_Output &output)
{
    if (output.which_mapping == proto_Output_wheelAxis_tag)
    {
        GamepadAxisType axis = wheel_gamepad_axis(output.mapping.wheelAxis);
        output.which_mapping = proto_Output_gamepadAxis_tag;
        output.mapping.gamepadAxis = axis;
    }
}
