#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

GuitarHeroDrumsButtonMapping::GuitarHeroDrumsButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void GuitarHeroDrumsButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void GuitarHeroDrumsButtonMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats probably
    WiiDrumDataFormat3_t *report = (WiiDrumDataFormat3_t *)buf;
    switch (m_mapping.mapping.ghDrumButton)
    {
    case GuitarHeroDrumsA:
        report->a |= m_lastValue;
        break;
    case GuitarHeroDrumsB:
        report->b |= m_lastValue;
        break;
    case GuitarHeroDrumsX:
        report->x |= m_lastValue;
        break;
    case GuitarHeroDrumsY:
        report->y |= m_lastValue;
        break;
    case GuitarHeroDrumsBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroDrumsStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroDrumsGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadUp:
        break;
    case GuitarHeroDrumsDpadDown:
        break;
    case GuitarHeroDrumsDpadLeft:
        break;
    case GuitarHeroDrumsDpadRight:
        break;
    }
}
void GuitarHeroDrumsButtonMapping::update_switch(uint8_t *buf)
{
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.ghDrumButton)
    {
    case GuitarHeroDrumsA:
        report->a |= m_lastValue;
        break;
    case GuitarHeroDrumsB:
        report->b |= m_lastValue;
        break;
    case GuitarHeroDrumsX:
        report->x |= m_lastValue;
        break;
    case GuitarHeroDrumsY:
        report->y |= m_lastValue;
        break;
    case GuitarHeroDrumsBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroDrumsStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroDrumsGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void GuitarHeroDrumsButtonMapping::update_ps2(uint8_t *buf)
{
    // Not a thing - drums were always usb here
}

void GuitarHeroDrumsButtonMapping::update_ps3(uint8_t *buf)
{
    PS3GuitarHeroDrums_Data_t *report = (PS3GuitarHeroDrums_Data_t *)buf;
    switch (m_mapping.mapping.ghDrumButton)
    {
    case GuitarHeroDrumsA:
        report->a |= m_lastValue;
        break;
    case GuitarHeroDrumsB:
        report->b |= m_lastValue;
        break;
    case GuitarHeroDrumsX:
        report->x |= m_lastValue;
        break;
    case GuitarHeroDrumsY:
        report->y |= m_lastValue;
        break;
    case GuitarHeroDrumsBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroDrumsStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroDrumsGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void GuitarHeroDrumsButtonMapping::update_ps4(uint8_t *buf)
{
}

void GuitarHeroDrumsButtonMapping::update_ps5(uint8_t *buf)
{
}

void GuitarHeroDrumsButtonMapping::update_xinput(uint8_t *buf)
{
    XInputGuitarHeroDrums_Data_t *report = (XInputGuitarHeroDrums_Data_t *)buf;
    switch (m_mapping.mapping.ghDrumButton)
    {
    case GuitarHeroDrumsA:
        report->a |= m_lastValue;
        break;
    case GuitarHeroDrumsB:
        report->b |= m_lastValue;
        break;
    case GuitarHeroDrumsX:
        report->x |= m_lastValue;
        break;
    case GuitarHeroDrumsY:
        report->y |= m_lastValue;
        break;
    case GuitarHeroDrumsBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroDrumsStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroDrumsGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void GuitarHeroDrumsButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGuitarHeroDrums_Data_t *report = (OGXboxGuitarHeroDrums_Data_t *)buf;
    switch (m_mapping.mapping.ghDrumButton)
    {
    case GuitarHeroDrumsA:
        report->a |= m_lastValue;
        break;
    case GuitarHeroDrumsB:
        report->b |= m_lastValue;
        break;
    case GuitarHeroDrumsX:
        report->x |= m_lastValue;
        break;
    case GuitarHeroDrumsY:
        report->y |= m_lastValue;
        break;
    case GuitarHeroDrumsBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroDrumsStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroDrumsDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void GuitarHeroDrumsButtonMapping::update_xboxone(uint8_t *buf)
{
}