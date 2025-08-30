#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

GuitarHeroGuitarAxisMapping::GuitarHeroGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : Mapping(id), m_mapping(mapping), m_input(std::move(input)), m_trigger(mapping.mapping.gamepadAxis == GamepadLeftTrigger || mapping.mapping.gamepadAxis == GamepadRightTrigger)
{
}

uint16_t calibrateGh(float val, float max, float min, float deadzone, float center, bool trigger)
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
void GuitarHeroGuitarAxisMapping::update(bool full_poll)
{
    auto val = m_input->tickAnalog();
    if (val != m_lastValue || full_poll)
    {
        m_lastValue = val;
        m_calibratedValue = calibrateGh(val, m_mapping.max, m_mapping.min, m_mapping.deadzone, m_mapping.center, m_trigger);
        proto_Event event = {which_event : proto_Event_axis_tag, event : {axis : {m_id, m_lastValue, m_calibratedValue}}};
        send_event(event);
    }
}
void GuitarHeroGuitarAxisMapping::update_hid(uint8_t *buf)
{
    PCGuitarHeroGuitar_Data_t *report = (PCGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    }
}
void GuitarHeroGuitarAxisMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats.
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        // report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarAxisMapping::update_switch(uint8_t *buf)
{
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarAxisMapping::update_ps2(uint8_t *buf)
{
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarAxisMapping::update_ps3(uint8_t *buf)
{
    PS3GuitarHeroGuitar_Data_t *report = (PS3GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
   case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    }
}

void GuitarHeroGuitarAxisMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    // case GuitarHeroGuitarLeftStickX:
    //     report->leftStickX = m_calibratedValue >> 8;
    //     break;
    // case GuitarHeroGuitarLeftStickY:
    //     report->leftStickY = m_calibratedValue >> 8;
    //     break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGuitarHeroGuitar_Data_t *report = (OGXboxGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    // case GuitarHeroGuitarLeftStickX:
    //     report->leftStickX = m_calibratedValue >> 8;
    //     break;
    // case GuitarHeroGuitarLeftStickY:
    //     report->leftStickY = m_calibratedValue >> 8;
    //     break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}