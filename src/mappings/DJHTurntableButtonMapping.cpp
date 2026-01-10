#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

DJHTurntableButtonMapping::DJHTurntableButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
    
}

void DJHTurntableButtonMapping::update_hid(uint8_t *buf)
{
    PCDJHTurntable_Data_t *report = (PCDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.djhButton)
    {
    case DJHTurntableLeftGreen:
        report->a |= m_lastValue;
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntableLeftRed:
        report->b |= m_lastValue;
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntableLeftBlue:
        report->x |= m_lastValue;
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntableRightGreen:
        report->a |= m_lastValue;
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntableRightRed:
        report->b |= m_lastValue;
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntableRightBlue:
        report->x |= m_lastValue;
        report->rightBlue |= m_lastValue;
        break;
    case DJHTurntableA:
        report->a |= m_lastValue;
        break;
    case DJHTurntableB:
        report->b |= m_lastValue;
        break;
    case DJHTurntableX:
        report->x |= m_lastValue;
        break;
    case DJHTurntableY:
        report->y |= m_lastValue;
        break;
    case DJHTurntableBack:
        report->back |= m_lastValue;
        break;
    case DJHTurntableStart:
        report->start |= m_lastValue;
        break;
    case DJHTurntableGuide:
        report->guide |= m_lastValue;
        break;
    case DJHTurntableDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case DJHTurntableDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case DJHTurntableDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case DJHTurntableDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void DJHTurntableButtonMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats probably
    WiiTurntableIntermediateFormat3_t *report = (WiiTurntableIntermediateFormat3_t *)buf;
    switch (m_mapping.mapping.djhButton)
    {
    case DJHTurntableLeftGreen:
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntableLeftRed:
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntableLeftBlue:
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntableRightGreen:
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntableRightRed:
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntableRightBlue:
        report->rightBlue |= m_lastValue;
        break;
    case DJHTurntableA:
        // TODO: if we end up doing fill wiimote emulation, then its worth mapping this to the remote
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntableB:
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntableX:
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntableY:
        report->y |= m_lastValue;
        break;
    case DJHTurntableBack:
        report->back |= m_lastValue;
        break;
    case DJHTurntableStart:
        report->start |= m_lastValue;
        break;
    case DJHTurntableGuide:
        report->guide |= m_lastValue;
        break;
    case DJHTurntableDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case DJHTurntableDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case DJHTurntableDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case DJHTurntableDpadRight:
        report->dpadRight |= m_lastValue;
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
    case DJHTurntableLeftGreen:
        report->a |= m_lastValue;
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntableLeftRed:
        report->b |= m_lastValue;
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntableLeftBlue:
        report->x |= m_lastValue;
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntableRightGreen:
        report->a |= m_lastValue;
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntableRightRed:
        report->b |= m_lastValue;
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntableRightBlue:
        report->x |= m_lastValue;
        report->rightBlue |= m_lastValue;
        break;
    case DJHTurntableA:
        report->a |= m_lastValue;
        break;
    case DJHTurntableB:
        report->b |= m_lastValue;
        break;
    case DJHTurntableX:
        report->x |= m_lastValue;
        break;
    case DJHTurntableY:
        report->y |= m_lastValue;
        break;
    case DJHTurntableBack:
        report->back |= m_lastValue;
        break;
    case DJHTurntableStart:
        report->start |= m_lastValue;
        break;
    case DJHTurntableGuide:
        report->guide |= m_lastValue;
        break;
    case DJHTurntableDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case DJHTurntableDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case DJHTurntableDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case DJHTurntableDpadRight:
        report->dpadRight |= m_lastValue;
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
    case DJHTurntableLeftGreen:
        report->a |= m_lastValue;
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntableLeftRed:
        report->b |= m_lastValue;
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntableLeftBlue:
        report->x |= m_lastValue;
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntableRightGreen:
        report->a |= m_lastValue;
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntableRightRed:
        report->b |= m_lastValue;
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntableRightBlue:
        report->x |= m_lastValue;
        report->rightBlue |= m_lastValue;
        break;
    case DJHTurntableA:
        report->a |= m_lastValue;
        break;
    case DJHTurntableB:
        report->b |= m_lastValue;
        break;
    case DJHTurntableX:
        report->x |= m_lastValue;
        break;
    case DJHTurntableY:
        report->y |= m_lastValue;
        break;
    case DJHTurntableBack:
        report->back |= m_lastValue;
        break;
    case DJHTurntableStart:
        report->start |= m_lastValue;
        break;
    case DJHTurntableGuide:
        report->guide |= m_lastValue;
        break;
    case DJHTurntableDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case DJHTurntableDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case DJHTurntableDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case DJHTurntableDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void DJHTurntableButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxDJHTurntable_Data_t *report = (OGXboxDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.djhButton)
    {
    case DJHTurntableLeftGreen:
        report->a |= m_lastValue;
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntableLeftRed:
        report->b |= m_lastValue;
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntableLeftBlue:
        report->x |= m_lastValue;
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntableRightGreen:
        report->a |= m_lastValue;
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntableRightRed:
        report->b |= m_lastValue;
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntableRightBlue:
        report->x |= m_lastValue;
        report->rightBlue |= m_lastValue;
        break;
    case DJHTurntableA:
        report->a |= m_lastValue;
        break;
    case DJHTurntableB:
        report->b |= m_lastValue;
        break;
    case DJHTurntableX:
        report->x |= m_lastValue;
        break;
    case DJHTurntableY:
        report->y |= m_lastValue;
        break;
    case DJHTurntableBack:
        report->back |= m_lastValue;
        break;
    case DJHTurntableStart:
        report->start |= m_lastValue;
        break;
    case DJHTurntableDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case DJHTurntableDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case DJHTurntableDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case DJHTurntableDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}