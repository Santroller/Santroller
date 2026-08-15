#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

RockBandDrumsAxisMapping::RockBandDrumsAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile, DrumState *state) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.rbDrumAxis != RockBandDrums_LeftStickX && mapping.mapping.rbDrumAxis != RockBandDrums_LeftStickY), state(state)
{
}

void RockBandDrumsAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void RockBandDrumsAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void RockBandDrumsAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing on switch
}

void RockBandDrumsAxisMapping::update_ps2(uint8_t *buf)
{
    // not a thing on ps2
}

void RockBandDrumsAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    if (state->cymbalGlitchFix && m_mapping.has_debounce)
    {
        if (m_mapping.mapping.rbDrumAxis >= RockBandDrums_GreenPad && m_mapping.mapping.rbDrumAxis <= RockBandDrums_GreenCymbal)
        {
            if (millis() - state->lastGlobalPoll > m_mapping.debounce) {
                state->lastDrum = RockBandDrums_RedPad;
            }
            if (state->lastDrum != RockBandDrums_RedPad && state->lastDrum != m_mapping.mapping.rbDrumAxis)
            {
                m_lastPoll = millis();
                return;
            }
            state->lastGlobalPoll = millis();
            state->lastDrum = m_mapping.mapping.rbDrumAxis;
        }
    }
    switch (m_mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        state->redPad = m_calibratedValue;
        break;
    case RockBandDrums_YellowPad:
        state->yellowPad = m_calibratedValue;
        break;
    case RockBandDrums_BluePad:
        state->bluePad = m_calibratedValue;
        break;
    case RockBandDrums_GreenPad:
        state->greenPad = m_calibratedValue;
        break;
    case RockBandDrums_YellowCymbal:
        state->yellowCymbal = m_calibratedValue;
        break;
    case RockBandDrums_BlueCymbal:
        state->blueCymbal = m_calibratedValue;
        break;
    case RockBandDrums_GreenCymbal:
        state->greenCymbal = m_calibratedValue;
        break;
    default:
        break;
    }
    PS3RockBandDrums_Data_t *report = (PS3RockBandDrums_Data_t *)buf;
    if (state->yellowCymbal && !state->yellowPad)
    {
        report->yellowVelocity = 0xFF - (state->yellowCymbal >> 8);
        report->y = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (state->yellowPad && !state->yellowCymbal)
    {
        report->yellowVelocity = 0xFF - (state->yellowPad >> 8);
        report->y = true;
        report->padFlag = true;
    }
    if (state->yellowPad && state->yellowCymbal && !state->redPad)
    {
        report->redVelocity = 0xFF - (state->yellowCymbal >> 8);
        report->yellowVelocity = 0xFF - (state->yellowCymbal >> 8);
        report->y = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (state->blueCymbal && !state->bluePad)
    {
        report->blueVelocity = 0xFF - (state->blueCymbal >> 8);
        report->x = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (state->bluePad && !state->blueCymbal)
    {
        report->blueVelocity = 0xFF - (state->bluePad >> 8);
        report->x = true;
        report->padFlag = true;
    }
    if (state->bluePad && state->blueCymbal && !state->redPad)
    {
        report->redVelocity = 0xFF - (state->blueCymbal >> 8);
        report->blueVelocity = 0xFF - (state->bluePad >> 8);
        report->x = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (state->greenCymbal && !state->greenPad)
    {
        report->greenVelocity = 0xFF - (state->greenCymbal >> 8);
        report->a = true;
        report->cymbalFlag = true;
    }
    if (state->greenPad && !state->greenCymbal)
    {
        report->greenVelocity = 0xFF - (state->greenPad >> 8);
        report->a = true;
        report->padFlag = true;
    }
    if (state->greenPad && state->greenCymbal && !state->redPad)
    {
        report->redVelocity = 0xFF - (state->greenCymbal >> 8);
        report->greenVelocity = 0xFF - (state->greenPad >> 8);
        report->a = true;
        report->padFlag = true;
        report->cymbalFlag = true;
    }
    if (state->redPad)
    {
        report->redVelocity = 0xFF - (state->redPad >> 8);
        report->b = true;
        report->padFlag = true;
    }
    switch (m_mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandDrums_LeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandDrums_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        report->l2 = true;
        break;
    case RockBandDrums_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        report->r2 = true;
        break;
    default:
        break;
    }
}

void RockBandDrumsAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS4RockBandDrums_Data_t *report = (PS4RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        report->redVelocity = m_calibratedValue >> 8;
        report->b = true;
        break;
    case RockBandDrums_YellowPad:
        report->yellowVelocity = m_calibratedValue >> 8;
        report->y = true;
        break;
    case RockBandDrums_BluePad:
        report->blueVelocity = m_calibratedValue >> 8;
        report->x = true;
        break;
    case RockBandDrums_GreenPad:
        report->greenVelocity = m_calibratedValue >> 8;
        report->a = true;
        break;
    case RockBandDrums_YellowCymbal:
        report->yellowCymbalVelocity = m_calibratedValue >> 8;
        report->y = true;
        break;
    case RockBandDrums_BlueCymbal:
        report->blueCymbalVelocity = m_calibratedValue >> 8;
        report->x = true;
        break;
    case RockBandDrums_GreenCymbal:
        report->greenCymbalVelocity = m_calibratedValue >> 8;
        report->a = true;
        break;
    case RockBandDrums_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandDrums_LeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandDrums_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        report->l2 = true;
        break;
    case RockBandDrums_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        report->r2 = true;
        break;
    default:
        break;
    }
}

void RockBandDrumsAxisMapping::update_ps5(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS5RockBandDrums_Data_t *report = (PS5RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        report->redVelocity = m_calibratedValue >> 8;
        report->b = true;
        break;
    case RockBandDrums_YellowPad:
        report->yellowVelocity = m_calibratedValue >> 8;
        report->y = true;
        break;
    case RockBandDrums_BluePad:
        report->blueVelocity = m_calibratedValue >> 8;
        report->x = true;
        break;
    case RockBandDrums_GreenPad:
        report->greenVelocity = m_calibratedValue >> 8;
        report->a = true;
        break;
    case RockBandDrums_YellowCymbal:
        report->yellowCymbalVelocity = m_calibratedValue >> 8;
        report->y = true;
        break;
    case RockBandDrums_BlueCymbal:
        report->blueCymbalVelocity = m_calibratedValue >> 8;
        report->x = true;
        break;
    case RockBandDrums_GreenCymbal:
        report->greenCymbalVelocity = m_calibratedValue >> 8;
        report->a = true;
        break;
    case RockBandDrums_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandDrums_LeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandDrums_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        report->l2 = true;
        break;
    case RockBandDrums_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        report->r2 = true;
        break;
    default:
        break;
    }
}

void RockBandDrumsAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    if (state->cymbalGlitchFix && m_mapping.has_debounce)
    {
        if (m_mapping.mapping.rbDrumAxis >= RockBandDrums_GreenPad && m_mapping.mapping.rbDrumAxis <= RockBandDrums_GreenCymbal)
        {
            if (millis() - state->lastGlobalPoll > m_mapping.debounce) {
                state->lastDrum = RockBandDrums_RedPad;
            }
            if (state->lastDrum != RockBandDrums_RedPad && state->lastDrum != m_mapping.mapping.rbDrumAxis)
            {
                m_lastPoll = millis();
                return;
            }
            state->lastGlobalPoll = millis();
            state->lastDrum = m_mapping.mapping.rbDrumAxis;
        }
    }
    switch (m_mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        state->redPad = m_calibratedValue;
        break;
    case RockBandDrums_YellowPad:
        state->yellowPad = m_calibratedValue;
        break;
    case RockBandDrums_BluePad:
        state->bluePad = m_calibratedValue;
        break;
    case RockBandDrums_GreenPad:
        state->greenPad = m_calibratedValue;
        break;
    case RockBandDrums_YellowCymbal:
        state->yellowCymbal = m_calibratedValue;
        break;
    case RockBandDrums_BlueCymbal:
        state->blueCymbal = m_calibratedValue;
        break;
    case RockBandDrums_GreenCymbal:
        state->greenCymbal = m_calibratedValue;
        break;
    default:
        break;
    }
    XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)buf;
    if (state->yellowCymbal && !state->yellowPad)
    {
        report->yellowVelocity = -((state->yellowCymbal >> 1) - 32768);
        report->y = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (state->yellowPad && !state->yellowCymbal)
    {
        report->yellowVelocity = -((state->yellowPad >> 1) - 32768);
        report->y = true;
        report->padFlag = true;
    }
    if (state->yellowPad && state->yellowCymbal && !state->redPad)
    {
        report->redVelocity = (state->yellowCymbal >> 1) - 32768;
        report->yellowVelocity = -((state->yellowCymbal >> 1) - 32768);
        report->y = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (state->blueCymbal && !state->bluePad)
    {
        report->blueVelocity = (state->blueCymbal >> 1) - 32768;
        report->x = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (state->bluePad && !state->blueCymbal)
    {
        report->blueVelocity = (state->bluePad >> 1) - 32768;
        report->x = true;
        report->padFlag = true;
    }
    if (state->bluePad && state->blueCymbal && !state->redPad)
    {
        report->redVelocity = (state->blueCymbal >> 1) - 32768;
        report->blueVelocity = (state->bluePad >> 1) - 32768;
        report->x = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (state->greenCymbal && !state->greenPad)
    {
        report->greenVelocity = -((state->greenCymbal >> 1) - 32768);
        report->a = true;
        report->cymbalFlag = true;
    }
    if (state->greenPad && !state->greenCymbal)
    {
        report->greenVelocity = -((state->greenPad >> 1) - 32768);
        report->a = true;
        report->padFlag = true;
    }
    if (state->greenPad && state->greenCymbal && !state->redPad)
    {
        report->redVelocity = (state->greenCymbal >> 1) - 32768;
        report->greenVelocity = -((state->greenPad >> 1) - 32768);
        report->a = true;
        report->padFlag = true;
        report->cymbalFlag = true;
    }
    if (state->redPad)
    {
        report->redVelocity = (state->redPad >> 1) - 32768;
        report->b = true;
        report->padFlag = true;
    }
    switch (m_mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_LeftStickX:
        if (!m_centered)
        {
            report->redVelocity = m_calibratedValue - 32768;
        }
        break;
    case RockBandDrums_LeftStickY:
        if (!m_centered)
        {
            report->yellowVelocity = m_calibratedValue - 32768;
        }
        break;
    case RockBandDrums_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case RockBandDrums_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void RockBandDrumsAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxRockBandDrums_Data_t *report = (OGXboxRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        report->redVelocity = m_calibratedValue - 32768;
        report->b = true;
        report->padFlag = true;
        break;
    case RockBandDrums_YellowPad:
        report->yellowVelocity = m_calibratedValue - 32768;
        report->y = true;
        report->padFlag = true;
        break;
    case RockBandDrums_BluePad:
        report->blueVelocity = m_calibratedValue - 32768;
        report->x = true;
        report->padFlag = true;
        break;
    case RockBandDrums_GreenPad:
        report->greenVelocity = m_calibratedValue - 32768;
        report->a = true;
        report->padFlag = true;
        break;
    case RockBandDrums_YellowCymbal:
        report->yellowVelocity = m_calibratedValue - 32768;
        report->y = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
        break;
    case RockBandDrums_BlueCymbal:
        report->blueVelocity = m_calibratedValue - 32768;
        report->x = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
        break;
    case RockBandDrums_GreenCymbal:
        report->greenVelocity = m_calibratedValue - 32768;
        report->a = true;
        report->cymbalFlag = true;
        break;
    case RockBandDrums_LeftStickX:
        if (!m_centered)
        {
            report->redVelocity = m_calibratedValue - 32768;
        }
        break;
    case RockBandDrums_LeftStickY:
        if (!m_centered)
        {
            report->yellowVelocity = m_calibratedValue - 32768;
        }
        break;
    default:
        break;
    }
}
void RockBandDrumsAxisMapping::update_xboxone(uint8_t *buf)
{

    if (m_centered)
    {
        return;
    }
    XboxOneRockBandDrums_Data_t *report = (XboxOneRockBandDrums_Data_t *)buf;

    switch (m_mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        report->redVelocity = m_calibratedValue >> 12;
        report->b = true;
        break;
    case RockBandDrums_YellowPad:
        report->yellowVelocity = m_calibratedValue >> 12;
        report->y = true;
        break;
    case RockBandDrums_BluePad:
        report->blueVelocity = m_calibratedValue >> 12;
        report->x = true;
        break;
    case RockBandDrums_GreenPad:
        report->greenVelocity = m_calibratedValue >> 12;
        report->a = true;
        break;
    case RockBandDrums_YellowCymbal:
        report->yellowCymbalVelocity = m_calibratedValue >> 12;
        report->y = true;
        break;
    case RockBandDrums_BlueCymbal:
        report->blueCymbalVelocity = m_calibratedValue >> 12;
        report->x = true;
        break;
    case RockBandDrums_GreenCymbal:
        report->greenCymbalVelocity = m_calibratedValue >> 12;
        report->a = true;
        break;
    // case RockBandDrums_LeftStickX:
    //     if (!m_centered)
    //     {
    //         report->redVelocity = m_calibratedValue - 32768;
    //     }
    //     break;
    // case RockBandDrums_LeftStickY:
    //     if (!m_centered)
    //     {
    //         report->yellowVelocity = m_calibratedValue - 32768;
    //     }
    //     break;
    default:
        break;
    }
}