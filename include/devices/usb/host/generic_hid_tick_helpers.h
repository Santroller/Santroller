#pragma once

#include "protocols/controller_reports.hpp"
#include "config.pb.h"

// Shared tick_digital and tick_analog implementations for HID devices we have no explicit support for.
// These operate on the USB_Host_Data_t produced by fill_generic_report

#define GENERIC_AXIS_CENTER 0x8000

inline bool generic_hid_tick_digital_impl(const USB_Host_Data_t &data, proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:               return (data.genericButtons & (1 << 0)) != 0;
        case Gamepad_B:               return (data.genericButtons & (1 << 1)) != 0;
        case Gamepad_X:               return (data.genericButtons & (1 << 2)) != 0;
        case Gamepad_Y:               return (data.genericButtons & (1 << 3)) != 0;
        case Gamepad_LeftShoulder:    return (data.genericButtons & (1 << 4)) != 0;
        case Gamepad_RightShoulder:   return (data.genericButtons & (1 << 5)) != 0;
        case Gamepad_Back:            return (data.genericButtons & (1 << 6)) != 0 || data.back;
        case Gamepad_Start:           return (data.genericButtons & (1 << 7)) != 0;
        case Gamepad_LeftThumbClick:  return (data.genericButtons & (1 << 8)) != 0;
        case Gamepad_RightThumbClick: return (data.genericButtons & (1 << 9)) != 0;
        // Guide / Capture may come from a button or a system / consumer usage (AC Home, Record)
        case Gamepad_Guide:           return (data.genericButtons & (1 << 10)) != 0 || data.guide;
        case Gamepad_Capture:         return (data.genericButtons & (1 << 11)) != 0 || data.capture;
        case Gamepad_DpadUp:          return data.dpadUp != 0;
        case Gamepad_DpadDown:        return data.dpadDown != 0;
        case Gamepad_DpadLeft:        return data.dpadLeft != 0;
        case Gamepad_DpadRight:       return data.dpadRight != 0;
        default:                      return false;
        }
    }
    return false;
}

// Sticks rest at the center until the device reports the axis
inline uint16_t generic_hid_stick(const USB_Host_Data_t &data, uint16_t axis, uint16_t value, bool invert)
{
    if (!(data.genericAxesPresent & axis))
    {
        return GENERIC_AXIS_CENTER;
    }
    return invert ? UINT16_MAX - value : value;
}

inline uint16_t generic_hid_trigger(const USB_Host_Data_t &data, uint16_t axis, uint16_t value)
{
    return (data.genericAxesPresent & axis) ? value : 0;
}

inline uint16_t generic_hid_tick_analog_impl(const USB_Host_Data_t &data, proto_Output &type)
{
    if (type.which_mapping != proto_Output_gamepadAxis_tag)
    {
        return 0;
    }
    uint16_t present = data.genericAxesPresent;
    // Most HID gamepads put the right stick on Z / Rz (and analog triggers on Rx / Ry, like the DS4).
    // Devices without Z / Rz tend to use Rx / Ry for the right stick and Z for the triggers instead
    bool right_on_z = (present & (GENERIC_AXIS_Z | GENERIC_AXIS_RZ)) == (GENERIC_AXIS_Z | GENERIC_AXIS_RZ) ||
                      (present & (GENERIC_AXIS_RX | GENERIC_AXIS_RY)) != (GENERIC_AXIS_RX | GENERIC_AXIS_RY);
    bool sim_triggers = present & (GENERIC_AXIS_BRAKE | GENERIC_AXIS_ACCELERATOR);
    // HID axes report down as the maximum, so Y is inverted
    switch (type.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        return generic_hid_stick(data, GENERIC_AXIS_X, data.genericAxisX, false);
    case Gamepad_LeftStickY:
        return generic_hid_stick(data, GENERIC_AXIS_Y, data.genericAxisY, true);
    case Gamepad_RightStickX:
        return right_on_z ? generic_hid_stick(data, GENERIC_AXIS_Z, data.genericAxisZ, false)
                          : generic_hid_stick(data, GENERIC_AXIS_RX, data.genericAxisRx, false);
    case Gamepad_RightStickY:
        return right_on_z ? generic_hid_stick(data, GENERIC_AXIS_RZ, data.genericAxisRz, true)
                          : generic_hid_stick(data, GENERIC_AXIS_RY, data.genericAxisRy, true);
    case Gamepad_LeftTrigger:
        if (sim_triggers)
        {
            return generic_hid_trigger(data, GENERIC_AXIS_BRAKE, data.genericAxisBrake);
        }
        return right_on_z ? generic_hid_trigger(data, GENERIC_AXIS_RX, data.genericAxisRx)
                          : generic_hid_trigger(data, GENERIC_AXIS_Z, data.genericAxisZ);
    case Gamepad_RightTrigger:
        if (sim_triggers)
        {
            return generic_hid_trigger(data, GENERIC_AXIS_ACCELERATOR, data.genericAxisAccelerator);
        }
        return right_on_z ? generic_hid_trigger(data, GENERIC_AXIS_RY, data.genericAxisRy)
                          : generic_hid_trigger(data, GENERIC_AXIS_RZ, data.genericAxisRz);
    default:
        return 0;
    }
}
