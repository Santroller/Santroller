#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

DJHTurntableButtonMapping::DJHTurntableButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void DJHTurntableButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void DJHTurntableButtonMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats probably
    WiiTurntableIntermediateFormat3_t *report = (WiiTurntableIntermediateFormat3_t *)buf;
    switch (m_mapping.mapping.djhButton)
    {
    case DJHTurntable_LeftGreen:
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntable_LeftRed:
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntable_LeftBlue:
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntable_RightGreen:
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntable_RightRed:
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntable_RightBlue:
        report->rightBlue |= m_lastValue;
        break;
    case DJHTurntable_A:
        // TODO: if we end up doing fill wiimote emulation, then its worth mapping this to the remote
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntable_B:
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntable_X:
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntable_Y:
        report->y |= m_lastValue;
        break;
    case DJHTurntable_Back:
        report->back |= m_lastValue;
        break;
    case DJHTurntable_Start:
        report->start |= m_lastValue;
        break;
    case DJHTurntable_Guide:
        report->guide |= m_lastValue;
        break;
    case DJHTurntable_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case DJHTurntable_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case DJHTurntable_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case DJHTurntable_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}
void DJHTurntableButtonMapping::update_switch(uint8_t *buf)
{
    // no switch turntable
}

void DJHTurntableButtonMapping::update_ps2(uint8_t *buf)
{
    // no ps2 turntable
}

void DJHTurntableButtonMapping::update_ps3(uint8_t *buf)
{
    PS3DJHTurntable_Data_t *report = (PS3DJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.djhButton)
    {
    case DJHTurntable_LeftGreen:
        report->a |= m_lastValue;
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntable_LeftRed:
        report->b |= m_lastValue;
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntable_LeftBlue:
        report->x |= m_lastValue;
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntable_RightGreen:
        report->a |= m_lastValue;
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntable_RightRed:
        report->b |= m_lastValue;
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntable_RightBlue:
        report->x |= m_lastValue;
        report->rightBlue |= m_lastValue;
        break;
    case DJHTurntable_A:
        report->a |= m_lastValue;
        break;
    case DJHTurntable_B:
        report->b |= m_lastValue;
        break;
    case DJHTurntable_X:
        report->x |= m_lastValue;
        break;
    case DJHTurntable_Y:
        report->y |= m_lastValue;
        break;
    case DJHTurntable_Back:
        report->back |= m_lastValue;
        break;
    case DJHTurntable_Start:
        report->start |= m_lastValue;
        break;
    case DJHTurntable_Guide:
        report->guide |= m_lastValue;
        break;
    case DJHTurntable_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case DJHTurntable_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case DJHTurntable_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case DJHTurntable_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}

void DJHTurntableButtonMapping::update_ps4(uint8_t *buf)
{
    // no ps4 turntable
}

void DJHTurntableButtonMapping::update_ps5(uint8_t *buf)
{
    // no ps5 turntable
}

void DJHTurntableButtonMapping::update_xinput(uint8_t *buf)
{
    XInputDJHTurntable_Data_t *report = (XInputDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.djhButton)
    {
    case DJHTurntable_LeftGreen:
        report->a |= m_lastValue;
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntable_LeftRed:
        report->b |= m_lastValue;
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntable_LeftBlue:
        report->x |= m_lastValue;
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntable_RightGreen:
        report->a |= m_lastValue;
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntable_RightRed:
        report->b |= m_lastValue;
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntable_RightBlue:
        report->x |= m_lastValue;
        report->rightBlue |= m_lastValue;
        break;
    case DJHTurntable_A:
        report->a |= m_lastValue;
        break;
    case DJHTurntable_B:
        report->b |= m_lastValue;
        break;
    case DJHTurntable_X:
        report->x |= m_lastValue;
        break;
    case DJHTurntable_Y:
        report->y |= m_lastValue;
        break;
    case DJHTurntable_Back:
        report->back |= m_lastValue;
        break;
    case DJHTurntable_Start:
        report->start |= m_lastValue;
        break;
    case DJHTurntable_Guide:
        report->guide |= m_lastValue;
        break;
    case DJHTurntable_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case DJHTurntable_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case DJHTurntable_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case DJHTurntable_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}
void DJHTurntableButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxDJHTurntable_Data_t *report = (OGXboxDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.djhButton)
    {
    case DJHTurntable_LeftGreen:
        report->a |= m_lastValue;
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntable_LeftRed:
        report->b |= m_lastValue;
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntable_LeftBlue:
        report->x |= m_lastValue;
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntable_RightGreen:
        report->a |= m_lastValue;
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntable_RightRed:
        report->b |= m_lastValue;
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntable_RightBlue:
        report->x |= m_lastValue;
        report->rightBlue |= m_lastValue;
        break;
    case DJHTurntable_A:
        report->a |= m_lastValue;
        break;
    case DJHTurntable_B:
        report->b |= m_lastValue;
        break;
    case DJHTurntable_X:
        report->x |= m_lastValue;
        break;
    case DJHTurntable_Y:
        report->y |= m_lastValue;
        break;
    case DJHTurntable_Back:
        report->back |= m_lastValue;
        break;
    case DJHTurntable_Start:
        report->start |= m_lastValue;
        break;
    case DJHTurntable_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case DJHTurntable_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case DJHTurntable_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case DJHTurntable_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}
void DJHTurntableButtonMapping::update_xboxone(uint8_t *buf)
{
    // not a thing
}