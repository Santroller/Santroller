#include "mappings/mapping.hpp"
#include "instance.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>


RockBandDrumsAxisMapping::RockBandDrumsAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
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
    if (m_profile->drum_state.cymbalGlitchFix && m_mapping.has_debounce)
    {
        if (m_mapping.mapping.mapping.rbDrumAxis >= RockBandDrums_GreenPad && m_mapping.mapping.mapping.rbDrumAxis <= RockBandDrums_GreenCymbal)
        {
            if (millis() - m_profile->drum_state.lastGlobalPoll > m_mapping.debounce) {
                m_profile->drum_state.lastDrum = RockBandDrums_RedPad;
            }
            if (m_profile->drum_state.lastDrum != RockBandDrums_RedPad && m_profile->drum_state.lastDrum != m_mapping.mapping.mapping.rbDrumAxis)
            {
                m_lastPoll = millis();
                return;
            }
            m_profile->drum_state.lastGlobalPoll = millis();
            m_profile->drum_state.lastDrum = m_mapping.mapping.mapping.rbDrumAxis;
        }
    }
    switch (m_mapping.mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        m_profile->drum_state.redPad = m_calibratedValue;
        break;
    case RockBandDrums_YellowPad:
        m_profile->drum_state.yellowPad = m_calibratedValue;
        break;
    case RockBandDrums_BluePad:
        m_profile->drum_state.bluePad = m_calibratedValue;
        break;
    case RockBandDrums_GreenPad:
        m_profile->drum_state.greenPad = m_calibratedValue;
        break;
    case RockBandDrums_YellowCymbal:
        m_profile->drum_state.yellowCymbal = m_calibratedValue;
        break;
    case RockBandDrums_BlueCymbal:
        m_profile->drum_state.blueCymbal = m_calibratedValue;
        break;
    case RockBandDrums_GreenCymbal:
        m_profile->drum_state.greenCymbal = m_calibratedValue;
        break;
    default:
        break;
    }
    PS3RockBandDrums_Data_t *report = (PS3RockBandDrums_Data_t *)buf;
    if (m_profile->drum_state.yellowCymbal && !m_profile->drum_state.yellowPad)
    {
        report->yellowVelocity = 0xFF - (m_profile->drum_state.yellowCymbal >> 8);
        report->y = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (m_profile->drum_state.yellowPad && !m_profile->drum_state.yellowCymbal)
    {
        report->yellowVelocity = 0xFF - (m_profile->drum_state.yellowPad >> 8);
        report->y = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.yellowPad && m_profile->drum_state.yellowCymbal && !m_profile->drum_state.redPad)
    {
        report->redVelocity = 0xFF - (m_profile->drum_state.yellowCymbal >> 8);
        report->yellowVelocity = 0xFF - (m_profile->drum_state.yellowCymbal >> 8);
        report->y = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (m_profile->drum_state.blueCymbal && !m_profile->drum_state.bluePad)
    {
        report->blueVelocity = 0xFF - (m_profile->drum_state.blueCymbal >> 8);
        report->x = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (m_profile->drum_state.bluePad && !m_profile->drum_state.blueCymbal)
    {
        report->blueVelocity = 0xFF - (m_profile->drum_state.bluePad >> 8);
        report->x = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.bluePad && m_profile->drum_state.blueCymbal && !m_profile->drum_state.redPad)
    {
        report->redVelocity = 0xFF - (m_profile->drum_state.blueCymbal >> 8);
        report->blueVelocity = 0xFF - (m_profile->drum_state.bluePad >> 8);
        report->x = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (m_profile->drum_state.greenCymbal && !m_profile->drum_state.greenPad)
    {
        report->greenVelocity = 0xFF - (m_profile->drum_state.greenCymbal >> 8);
        report->a = true;
        report->cymbalFlag = true;
    }
    if (m_profile->drum_state.greenPad && !m_profile->drum_state.greenCymbal)
    {
        report->greenVelocity = 0xFF - (m_profile->drum_state.greenPad >> 8);
        report->a = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.greenPad && m_profile->drum_state.greenCymbal && !m_profile->drum_state.redPad)
    {
        report->redVelocity = 0xFF - (m_profile->drum_state.greenCymbal >> 8);
        report->greenVelocity = 0xFF - (m_profile->drum_state.greenPad >> 8);
        report->a = true;
        report->padFlag = true;
        report->cymbalFlag = true;
    }
    if (m_profile->drum_state.redPad)
    {
        report->redVelocity = 0xFF - (m_profile->drum_state.redPad >> 8);
        report->b = true;
        report->padFlag = true;
    }
}

void RockBandDrumsAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS4RockBandDrums_Data_t *report = (PS4RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbDrumAxis)
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
    switch (m_mapping.mapping.mapping.rbDrumAxis)
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
    if (m_profile->drum_state.cymbalGlitchFix && m_mapping.has_debounce)
    {
        if (m_mapping.mapping.mapping.rbDrumAxis >= RockBandDrums_GreenPad && m_mapping.mapping.mapping.rbDrumAxis <= RockBandDrums_GreenCymbal)
        {
            if (millis() - m_profile->drum_state.lastGlobalPoll > m_mapping.debounce) {
                m_profile->drum_state.lastDrum = RockBandDrums_RedPad;
            }
            if (m_profile->drum_state.lastDrum != RockBandDrums_RedPad && m_profile->drum_state.lastDrum != m_mapping.mapping.mapping.rbDrumAxis)
            {
                m_lastPoll = millis();
                return;
            }
            m_profile->drum_state.lastGlobalPoll = millis();
            m_profile->drum_state.lastDrum = m_mapping.mapping.mapping.rbDrumAxis;
        }
    }
    switch (m_mapping.mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        m_profile->drum_state.redPad = m_calibratedValue;
        break;
    case RockBandDrums_YellowPad:
        m_profile->drum_state.yellowPad = m_calibratedValue;
        break;
    case RockBandDrums_BluePad:
        m_profile->drum_state.bluePad = m_calibratedValue;
        break;
    case RockBandDrums_GreenPad:
        m_profile->drum_state.greenPad = m_calibratedValue;
        break;
    case RockBandDrums_YellowCymbal:
        m_profile->drum_state.yellowCymbal = m_calibratedValue;
        break;
    case RockBandDrums_BlueCymbal:
        m_profile->drum_state.blueCymbal = m_calibratedValue;
        break;
    case RockBandDrums_GreenCymbal:
        m_profile->drum_state.greenCymbal = m_calibratedValue;
        break;
    default:
        break;
    }
    XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)buf;
    if (m_profile->drum_state.yellowCymbal && !m_profile->drum_state.yellowPad)
    {
        report->yellowVelocity = -((m_profile->drum_state.yellowCymbal >> 1) - 32768);
        report->y = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (m_profile->drum_state.yellowPad && !m_profile->drum_state.yellowCymbal)
    {
        report->yellowVelocity = -((m_profile->drum_state.yellowPad >> 1) - 32768);
        report->y = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.yellowPad && m_profile->drum_state.yellowCymbal && !m_profile->drum_state.redPad)
    {
        report->redVelocity = (m_profile->drum_state.yellowCymbal >> 1) - 32768;
        report->yellowVelocity = -((m_profile->drum_state.yellowCymbal >> 1) - 32768);
        report->y = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (m_profile->drum_state.blueCymbal && !m_profile->drum_state.bluePad)
    {
        report->blueVelocity = (m_profile->drum_state.blueCymbal >> 1) - 32768;
        report->x = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (m_profile->drum_state.bluePad && !m_profile->drum_state.blueCymbal)
    {
        report->blueVelocity = (m_profile->drum_state.bluePad >> 1) - 32768;
        report->x = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.bluePad && m_profile->drum_state.blueCymbal && !m_profile->drum_state.redPad)
    {
        report->redVelocity = (m_profile->drum_state.blueCymbal >> 1) - 32768;
        report->blueVelocity = (m_profile->drum_state.bluePad >> 1) - 32768;
        report->x = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (m_profile->drum_state.greenCymbal && !m_profile->drum_state.greenPad)
    {
        report->greenVelocity = -((m_profile->drum_state.greenCymbal >> 1) - 32768);
        report->a = true;
        report->cymbalFlag = true;
    }
    if (m_profile->drum_state.greenPad && !m_profile->drum_state.greenCymbal)
    {
        report->greenVelocity = -((m_profile->drum_state.greenPad >> 1) - 32768);
        report->a = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.greenPad && m_profile->drum_state.greenCymbal && !m_profile->drum_state.redPad)
    {
        report->redVelocity = (m_profile->drum_state.greenCymbal >> 1) - 32768;
        report->greenVelocity = -((m_profile->drum_state.greenPad >> 1) - 32768);
        report->a = true;
        report->padFlag = true;
        report->cymbalFlag = true;
    }
    if (m_profile->drum_state.redPad)
    {
        report->redVelocity = (m_profile->drum_state.redPad >> 1) - 32768;
        report->b = true;
        report->padFlag = true;
    }
}
void RockBandDrumsAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxRockBandDrums_Data_t *report = (OGXboxRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbDrumAxis)
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

    switch (m_mapping.mapping.mapping.rbDrumAxis)
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
RockBandDrumsGamepadAxisMapping::RockBandDrumsGamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadAxisMapping(mapping, std::move(input), id, profile)
{
}
void RockBandDrumsGamepadAxisMapping::update_xboxone(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XboxOneRockBandDrums_Data_t *report = (XboxOneRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        // report->joystickX = m_calibratedValue - 32768;
        break;
    case Gamepad_LeftStickY:
        // report->joystickY = m_calibratedValue - 32768;
        break;
    case Gamepad_RightStickX:
        // report->rightStickX = m_calibratedValue - 32768;
        break;
    case Gamepad_RightStickY:
        // report->rightStickY = m_calibratedValue - 32768;
        break;
    case Gamepad_LeftTrigger:
        // report->leftTrigger = m_calibratedValue >> 6;
        break;
    case Gamepad_RightTrigger:
        // report->rightTrigger = m_calibratedValue >> 6;
        break;
    default:
        break;
    }
}