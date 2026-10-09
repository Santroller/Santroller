#pragma once
#include <stdint.h>
#include <utils.h>
#include "input.pb.h"

// Scale a raw input value onto the full 0 - 65535 output range.
// Triggers rest at 0 and use min + deadzone as their start point, sticks rest at the
// centre of the range and treat anything within deadzone of center as centred.
// min > max inverts the axis.
inline uint16_t calibrate_axis(float val, float max, float min, float deadzone, float center, bool trigger)
{
    if (trigger)
    {
        auto inverted = min > max;
        if (inverted)
        {
            min -= deadzone;
            if (val > min)
                return 0;
            if (val < max)
                val = max;
        }
        else
        {
            min += deadzone;
            if (val < min)
                return 0;
            if (val > max)
                val = max;
        }
        val = map(val, min, max, 0, UINT16_MAX);
    }
    else
    {

        auto inverted = min > max;
        if (inverted)
        {
            if (val < center)
            {
                if (center - val < deadzone)
                {
                    return UINT16_MAX / 2;
                }

                val = map(val, center - deadzone, max, UINT16_MAX / 2, UINT16_MAX);
            }
            else
            {
                if (val - center < deadzone)
                {
                    return UINT16_MAX / 2;
                }

                val = map(val, min, center + deadzone, 0, UINT16_MAX / 2);
            }
        }
        else
        {
            if (val < center)
            {
                if (center - val < deadzone)
                {
                    return UINT16_MAX / 2;
                }

                val = map(val, min, center - deadzone, 0, UINT16_MAX / 2);
            }
            else
            {
                if (val - center < deadzone)
                {
                    return UINT16_MAX / 2;
                }

                val = map(val, center + deadzone, max, UINT16_MAX / 2, UINT16_MAX);
            }
        }
    }
    if (val > UINT16_MAX)
        val = UINT16_MAX;
    if (val < 0)
        val = 0;
    return val;
}

// Whether an analog value counts as pressed for a button mapped from an analog input
inline bool analog_trigger_pressed(proto_AnalogToDigitalTriggerType trigger, int32_t val, int32_t trigger_value, int32_t max_trigger_value)
{
    if (trigger == AnalogToDigitalTriggerType_JoyHigh)
    {
        return val > trigger_value;
    }
    else if (trigger == AnalogToDigitalTriggerType_JoyLow)
    {
        return val < trigger_value;
    }
    else if (trigger == AnalogToDigitalTriggerType_Exact)
    {
        return val == trigger_value;
    }
    else if (trigger == AnalogToDigitalTriggerType_Range)
    {
        return val > trigger_value && val < max_trigger_value;
    }
    return false;
}
