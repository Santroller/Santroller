#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config.hpp"

ProjectDivaButtonMapping::ProjectDivaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
}

void ProjectDivaButtonMapping::update_hid(uint8_t *buf)
{
    PCGamepad_Data_t *report = (PCGamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaButton)
    {
    case ProjectDivaA:
        report->a = m_lastValue;
        break;
    case ProjectDivaB:
        report->b = m_lastValue;
        break;
    case ProjectDivaX:
        report->x = m_lastValue;
        break;
    case ProjectDivaY:
        report->y = m_lastValue;
        break;
    case ProjectDivaStart:
        report->start = m_lastValue;
        break;
    case ProjectDivaBack:
        report->back = m_lastValue;
        break;
    case ProjectDivaGuide:
        report->guide = m_lastValue;
        break;
    case ProjectDivaCapture:
        report->capture = m_lastValue;
        break;
    case ProjectDivaLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case ProjectDivaRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case ProjectDivaLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case ProjectDivaRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case ProjectDivaDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case ProjectDivaDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case ProjectDivaDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case ProjectDivaDpadRight:
        report->dpadRight = m_lastValue;
        break;
    }
}
void ProjectDivaButtonMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats.
    WiiClassicDataFormat3_t *report = (WiiClassicDataFormat3_t *)buf;
    switch (m_mapping.mapping.divaButton)
    {
    case ProjectDivaA:
        report->a = m_lastValue;
        break;
    case ProjectDivaB:
        report->b = m_lastValue;
        break;
    case ProjectDivaX:
        report->x = m_lastValue;
        break;
    case ProjectDivaY:
        report->y = m_lastValue;
        break;
    case ProjectDivaStart:
        report->start = m_lastValue;
        break;
    case ProjectDivaBack:
        report->back = m_lastValue;
        break;
    case ProjectDivaGuide:
        report->guide = m_lastValue;
        break;
    case ProjectDivaLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case ProjectDivaRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case ProjectDivaDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case ProjectDivaDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case ProjectDivaDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case ProjectDivaDpadRight:
        report->dpadRight = m_lastValue;
        break;
    default:
        break;
    }
}

void ProjectDivaButtonMapping::update_switch(uint8_t *buf)
{
    SwitchGamepad_Data_t *report = (SwitchGamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaButton)
    {
    case ProjectDivaA:
        report->a = m_lastValue;
        break;
    case ProjectDivaB:
        report->b = m_lastValue;
        break;
    case ProjectDivaX:
        report->x = m_lastValue;
        break;
    case ProjectDivaY:
        report->y = m_lastValue;
        break;
    case ProjectDivaStart:
        report->start = m_lastValue;
        break;
    case ProjectDivaBack:
        report->back = m_lastValue;
        break;
    case ProjectDivaGuide:
        report->guide = m_lastValue;
        break;
    case ProjectDivaCapture:
        report->capture = m_lastValue;
        break;
    case ProjectDivaLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case ProjectDivaRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case ProjectDivaLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case ProjectDivaRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case ProjectDivaDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case ProjectDivaDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case ProjectDivaDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case ProjectDivaDpadRight:
        report->dpadRight = m_lastValue;
        break;
    }
}
void ProjectDivaButtonMapping::update_ps2(uint8_t *buf)
{
    // TODO: pressures
    PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaButton)
    {
    case ProjectDivaA:
        report->a = m_lastValue;
        break;
    case ProjectDivaB:
        report->b = m_lastValue;
        break;
    case ProjectDivaX:
        report->x = m_lastValue;
        break;
    case ProjectDivaY:
        report->y = m_lastValue;
        break;
    case ProjectDivaStart:
        report->start = m_lastValue;
        break;
    case ProjectDivaBack:
        report->back = m_lastValue;
        break;
    case ProjectDivaLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case ProjectDivaRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case ProjectDivaLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case ProjectDivaRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case ProjectDivaDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case ProjectDivaDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case ProjectDivaDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case ProjectDivaDpadRight:
        report->dpadRight = m_lastValue;
        break;
    default:
        break;
    }
}
void ProjectDivaButtonMapping::update_ps3(uint8_t *buf)
{
    if (mode == ConsoleMode::Ps3)
    {
        // TODO: pressures
        PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)buf;
        switch (m_mapping.mapping.divaButton)
        {
        case ProjectDivaA:
            report->a = m_lastValue;
            break;
        case ProjectDivaB:
            report->b = m_lastValue;
            break;
        case ProjectDivaX:
            report->x = m_lastValue;
            break;
        case ProjectDivaY:
            report->y = m_lastValue;
            break;
        case ProjectDivaStart:
            report->start = m_lastValue;
            break;
        case ProjectDivaBack:
            report->back = m_lastValue;
            break;
        case ProjectDivaGuide:
            report->guide = m_lastValue;
            break;
        case ProjectDivaLeftShoulder:
            report->leftShoulder = m_lastValue;
            break;
        case ProjectDivaRightShoulder:
            report->rightShoulder = m_lastValue;
            break;
        case ProjectDivaLeftThumbClick:
            report->leftThumbClick = m_lastValue;
            break;
        case ProjectDivaRightThumbClick:
            report->rightThumbClick = m_lastValue;
            break;
        case ProjectDivaDpadUp:
            report->dpadUp = m_lastValue;
            break;
        case ProjectDivaDpadDown:
            report->dpadDown = m_lastValue;
            break;
        case ProjectDivaDpadLeft:
            report->dpadLeft = m_lastValue;
            break;
        case ProjectDivaDpadRight:
            report->dpadRight = m_lastValue;
            break;
        default:
            break;
        }
        return;
    }
    PS3SimpleGamepad_Data_t *report = (PS3SimpleGamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaButton)
    {
    case ProjectDivaA:
        report->a = m_lastValue;
        break;
    case ProjectDivaB:
        report->b = m_lastValue;
        break;
    case ProjectDivaX:
        report->x = m_lastValue;
        break;
    case ProjectDivaY:
        report->y = m_lastValue;
        break;
    case ProjectDivaStart:
        report->start = m_lastValue;
        break;
    case ProjectDivaBack:
        report->back = m_lastValue;
        break;
    case ProjectDivaGuide:
        report->guide = m_lastValue;
        break;
    case ProjectDivaLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case ProjectDivaRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case ProjectDivaLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case ProjectDivaRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case ProjectDivaDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case ProjectDivaDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case ProjectDivaDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case ProjectDivaDpadRight:
        report->dpadRight = m_lastValue;
        break;
    default:
        break;
    }
}
void ProjectDivaButtonMapping::update_ps4(uint8_t *buf)
{
    PS4Gamepad_Data_t *report = (PS4Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaButton)
    {
    case ProjectDivaA:
        report->a = m_lastValue;
        break;
    case ProjectDivaB:
        report->b = m_lastValue;
        break;
    case ProjectDivaX:
        report->x = m_lastValue;
        break;
    case ProjectDivaY:
        report->y = m_lastValue;
        break;
    case ProjectDivaStart:
        report->start = m_lastValue;
        break;
    case ProjectDivaBack:
        report->back = m_lastValue;
        break;
    case ProjectDivaGuide:
        report->guide = m_lastValue;
        break;
    case ProjectDivaLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case ProjectDivaRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case ProjectDivaLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case ProjectDivaRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case ProjectDivaDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case ProjectDivaDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case ProjectDivaDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case ProjectDivaDpadRight:
        report->dpadRight = m_lastValue;
        break;
    default:
        break;
    }
}
void ProjectDivaButtonMapping::update_xinput(uint8_t *buf)
{
    XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaButton)
    {
    case ProjectDivaA:
        report->a = m_lastValue;
        break;
    case ProjectDivaB:
        report->b = m_lastValue;
        break;
    case ProjectDivaX:
        report->x = m_lastValue;
        break;
    case ProjectDivaY:
        report->y = m_lastValue;
        break;
    case ProjectDivaStart:
        report->start = m_lastValue;
        break;
    case ProjectDivaBack:
        report->back = m_lastValue;
        break;
    case ProjectDivaGuide:
        report->guide = m_lastValue;
        break;
    case ProjectDivaLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case ProjectDivaRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case ProjectDivaLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case ProjectDivaRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case ProjectDivaDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case ProjectDivaDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case ProjectDivaDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case ProjectDivaDpadRight:
        report->dpadRight = m_lastValue;
        break;
    default:
        break;
    }
}
void ProjectDivaButtonMapping::update_ogxbox(uint8_t *buf)
{
    // TODO: pressures
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaButton)
    {
    case ProjectDivaA:
        report->a = m_lastValue;
        break;
    case ProjectDivaB:
        report->b = m_lastValue;
        break;
    case ProjectDivaX:
        report->x = m_lastValue;
        break;
    case ProjectDivaY:
        report->y = m_lastValue;
        break;
    case ProjectDivaStart:
        report->start = m_lastValue;
        break;
    case ProjectDivaBack:
        report->back = m_lastValue;
        break;
    case ProjectDivaLeftShoulder:
        report->leftShoulder = m_lastValue;
        break;
    case ProjectDivaRightShoulder:
        report->rightShoulder = m_lastValue;
        break;
    case ProjectDivaLeftThumbClick:
        report->leftThumbClick = m_lastValue;
        break;
    case ProjectDivaRightThumbClick:
        report->rightThumbClick = m_lastValue;
        break;
    case ProjectDivaDpadUp:
        report->dpadUp = m_lastValue;
        break;
    case ProjectDivaDpadDown:
        report->dpadDown = m_lastValue;
        break;
    case ProjectDivaDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case ProjectDivaDpadRight:
        report->dpadRight = m_lastValue;
        break;
    default:
        break;
    }
}