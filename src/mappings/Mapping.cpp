#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>
uint16_t Mapping::calibrate(float val, float max, float min, float deadzone, float center, bool trigger)
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
    if (val > UINT16_MAX)
        val = UINT16_MAX;
    if (val < 0)
        val = 0;
    return val;
}