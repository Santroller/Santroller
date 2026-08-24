#include "mappings/mapping.hpp"
#include "instance.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

DJHTurntableButtonMapping::DJHTurntableButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void DJHTurntableButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void DJHTurntableButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // TODO: we have to deal with data formats probably
    WiiTurntableIntermediateFormat3_t *report = (WiiTurntableIntermediateFormat3_t *)buf;
    switch (m_mapping.mapping.mapping.djhButton)
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
    switch (m_mapping.mapping.mapping.djhButton)
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
    switch (m_mapping.mapping.mapping.djhButton)
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
    default:
        break;
    }
}
void DJHTurntableButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxDJHTurntable_Data_t *report = (OGXboxDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.mapping.djhButton)
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
    default:
        break;
    }
}
void DJHTurntableButtonMapping::update_xboxone(uint8_t *buf)
{
    // not a thing
}