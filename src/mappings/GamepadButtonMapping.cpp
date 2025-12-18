#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config.hpp"

GamepadButtonMapping::GamepadButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
}

void GamepadButtonMapping::update_hid(uint8_t *buf)
{
    PCGamepad_Data_t *report = (PCGamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case GamepadA:
        report->a = m_lastValue;
        break;
    case GamepadB:
        report->b = m_lastValue;
        break;
    case GamepadX:
        report->x = m_lastValue;
        break;
    case GamepadY:
        report->y = m_lastValue;
        break;
    case GamepadStart:
        report->start = m_lastValue;
        break;
    case GamepadBack:
        report->back = m_lastValue;
        break;
    case GamepadGuide:
        report->guide = m_lastValue;
        break;
    case GamepadCapture:
        report->capture = m_lastValue;
        break;
    case GamepadLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case GamepadRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case GamepadLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case GamepadRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case GamepadDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case GamepadDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case GamepadDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case GamepadDpadRight:
        report->dpadRight = m_lastValue;
        break;
    }
}
void GamepadButtonMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats.
    WiiClassicDataFormat3_t *report = (WiiClassicDataFormat3_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case GamepadA:
        report->a = m_lastValue;
        break;
    case GamepadB:
        report->b = m_lastValue;
        break;
    case GamepadX:
        report->x = m_lastValue;
        break;
    case GamepadY:
        report->y = m_lastValue;
        break;
    case GamepadStart:
        report->start = m_lastValue;
        break;
    case GamepadBack:
        report->back = m_lastValue;
        break;
    case GamepadGuide:
        report->guide = m_lastValue;
        break;
    case GamepadLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case GamepadRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case GamepadDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case GamepadDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case GamepadDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case GamepadDpadRight:
        report->dpadRight = m_lastValue;
        break;
    default:
        break;
    }
}

void GamepadButtonMapping::update_switch(uint8_t *buf)
{
    SwitchGamepad_Data_t *report = (SwitchGamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case GamepadA:
        report->a = m_lastValue;
        break;
    case GamepadB:
        report->b = m_lastValue;
        break;
    case GamepadX:
        report->x = m_lastValue;
        break;
    case GamepadY:
        report->y = m_lastValue;
        break;
    case GamepadStart:
        report->start = m_lastValue;
        break;
    case GamepadBack:
        report->back = m_lastValue;
        break;
    case GamepadGuide:
        report->guide = m_lastValue;
        break;
    case GamepadCapture:
        report->capture = m_lastValue;
        break;
    case GamepadLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case GamepadRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case GamepadLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case GamepadRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case GamepadDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case GamepadDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case GamepadDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case GamepadDpadRight:
        report->dpadRight = m_lastValue;
        break;
    }
}
void GamepadButtonMapping::update_ps2(uint8_t *buf)
{
    // TODO: pressures
    PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case GamepadA:
        report->a = m_lastValue;
        break;
    case GamepadB:
        report->b = m_lastValue;
        break;
    case GamepadX:
        report->x = m_lastValue;
        break;
    case GamepadY:
        report->y = m_lastValue;
        break;
    case GamepadStart:
        report->start = m_lastValue;
        break;
    case GamepadBack:
        report->back = m_lastValue;
        break;
    case GamepadLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case GamepadRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case GamepadLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case GamepadRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case GamepadDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case GamepadDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case GamepadDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case GamepadDpadRight:
        report->dpadRight = m_lastValue;
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
        case GamepadA:
            report->a = m_lastValue;
            break;
        case GamepadB:
            report->b = m_lastValue;
            break;
        case GamepadX:
            report->x = m_lastValue;
            break;
        case GamepadY:
            report->y = m_lastValue;
            break;
        case GamepadStart:
            report->start = m_lastValue;
            break;
        case GamepadBack:
            report->back = m_lastValue;
            break;
        case GamepadGuide:
            report->guide = m_lastValue;
            break;
        case GamepadLeftShoulder:
            report->leftShoulder = m_lastValue;
            break;
        case GamepadRightShoulder:
            report->rightShoulder = m_lastValue;
            break;
        case GamepadLeftThumbClick:
            report->leftThumbClick = m_lastValue;
            break;
        case GamepadRightThumbClick:
            report->rightThumbClick = m_lastValue;
            break;
        case GamepadDpadUp:
            report->dpadUp = m_lastValue;
            break;
        case GamepadDpadDown:
            report->dpadDown = m_lastValue;
            break;
        case GamepadDpadLeft:
            report->dpadLeft = m_lastValue;
            break;
        case GamepadDpadRight:
            report->dpadRight = m_lastValue;
            break;
        default:
            break;
        }
        return;
    }
    PS3SimpleGamepad_Data_t *report = (PS3SimpleGamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadButton)
    {
    case GamepadA:
        report->a = m_lastValue;
        break;
    case GamepadB:
        report->b = m_lastValue;
        break;
    case GamepadX:
        report->x = m_lastValue;
        break;
    case GamepadY:
        report->y = m_lastValue;
        break;
    case GamepadStart:
        report->start = m_lastValue;
        break;
    case GamepadBack:
        report->back = m_lastValue;
        break;
    case GamepadGuide:
        report->guide = m_lastValue;
        break;
    case GamepadLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case GamepadRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case GamepadLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case GamepadRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case GamepadDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case GamepadDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case GamepadDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case GamepadDpadRight:
        report->dpadRight = m_lastValue;
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
    case GamepadA:
        report->a = m_lastValue;
        break;
    case GamepadB:
        report->b = m_lastValue;
        break;
    case GamepadX:
        report->x = m_lastValue;
        break;
    case GamepadY:
        report->y = m_lastValue;
        break;
    case GamepadStart:
        report->start = m_lastValue;
        break;
    case GamepadBack:
        report->back = m_lastValue;
        break;
    case GamepadGuide:
        report->guide = m_lastValue;
        break;
    case GamepadLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case GamepadRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case GamepadLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case GamepadRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case GamepadDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case GamepadDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case GamepadDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case GamepadDpadRight:
        report->dpadRight = m_lastValue;
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
    case GamepadA:
        report->a = m_lastValue;
        break;
    case GamepadB:
        report->b = m_lastValue;
        break;
    case GamepadX:
        report->x = m_lastValue;
        break;
    case GamepadY:
        report->y = m_lastValue;
        break;
    case GamepadStart:
        report->start = m_lastValue;
        break;
    case GamepadBack:
        report->back = m_lastValue;
        break;
    case GamepadGuide:
        report->guide = m_lastValue;
        break;
    case GamepadLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case GamepadRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case GamepadLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case GamepadRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case GamepadDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case GamepadDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case GamepadDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case GamepadDpadRight:
        report->dpadRight = m_lastValue;
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
    case GamepadA:
        report->a = m_lastValue;
        break;
    case GamepadB:
        report->b = m_lastValue;
        break;
    case GamepadX:
        report->x = m_lastValue;
        break;
    case GamepadY:
        report->y = m_lastValue;
        break;
    case GamepadStart:
        report->start = m_lastValue;
        break;
    case GamepadBack:
        report->back = m_lastValue;
        break;
    case GamepadLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case GamepadRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case GamepadLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case GamepadRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case GamepadDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case GamepadDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case GamepadDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case GamepadDpadRight:
        report->dpadRight = m_lastValue;
        break;
    default:
        break;
    }
}