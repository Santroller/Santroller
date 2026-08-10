#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config.hpp"

const uint8_t GamepadButtonMapping::dpad_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
GamepadButtonMapping::GamepadButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void GamepadButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void GamepadButtonMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats.
    WiiClassicDataFormat3_t *report = (WiiClassicDataFormat3_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Capture:
        // report->capture |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}

void GamepadButtonMapping::update_switch(uint8_t *buf)
{
    SwitchInputReport *report = (SwitchInputReport *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        report->capture |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void GamepadButtonMapping::update_ps2(uint8_t *buf)
{
    // TODO: pressures
    PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Capture:
        // report->capture |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}
void GamepadButtonMapping::update_ps3(uint8_t *buf)
{
    if (mode == ModePs3)
    {
        // TODO: pressures
        PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)buf;
        switch (m_mapping.mapping.gamepadButton)
        {
        case Gamepad_A:
            report->a |= m_lastValue;
            break;
        case Gamepad_B:
            report->b |= m_lastValue;
            break;
        case Gamepad_X:
            report->x |= m_lastValue;
            break;
        case Gamepad_Y:
            report->y |= m_lastValue;
            break;
        case Gamepad_Start:
            report->start |= m_lastValue;
            break;
        case Gamepad_Back:
            report->back |= m_lastValue;
            break;
        case Gamepad_Guide:
            report->guide |= m_lastValue;
            break;
        case Gamepad_Capture:
            report->capture |= m_lastValue;
            break;
        case Gamepad_LeftShoulder:
            report->leftShoulder |= m_lastValue;
            break;
        case Gamepad_RightShoulder:
            report->rightShoulder |= m_lastValue;
            break;
        case Gamepad_LeftThumbClick:
            report->leftThumbClick |= m_lastValue;
            break;
        case Gamepad_RightThumbClick:
            report->rightThumbClick |= m_lastValue;
            break;
        case Gamepad_DpadUp:
            report->dpadUp |= m_lastValue;
            break;
        case Gamepad_DpadDown:
            report->dpadDown |= m_lastValue;
            break;
        case Gamepad_DpadLeft:
            report->dpadLeft |= m_lastValue;
            break;
        case Gamepad_DpadRight:
            report->dpadRight |= m_lastValue;
            break;
        default:
            break;
        }
        return;
    }
    PS3ThirdPartyGamepad_Data_t *report = (PS3ThirdPartyGamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        report->capture |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}
void GamepadButtonMapping::update_ps4(uint8_t *buf)
{
    PS4Gamepad_Data_t *report = (PS4Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        report->capture |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}
void GamepadButtonMapping::update_ps5(uint8_t *buf)
{
    PS5Gamepad_Data_t *report = (PS5Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        report->touchpad |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}
void GamepadButtonMapping::update_xinput(uint8_t *buf)
{
    XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        report->capture |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}
void GamepadButtonMapping::update_ogxbox(uint8_t *buf)
{
    // TODO: pressures
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}
void GamepadButtonMapping::update_xboxone(uint8_t *buf)
{
    XboxOneGamepad_Data_t *report = (XboxOneGamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        if (m_lastValue)
        {
            report->consoleFunctions[0] = 0x01;
        }
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}