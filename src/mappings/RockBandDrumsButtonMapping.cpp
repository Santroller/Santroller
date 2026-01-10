#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

RockBandDrumsButtonMapping::RockBandDrumsButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
}

void RockBandDrumsButtonMapping::update_hid(uint8_t *buf)
{
    PCRockBandDrums_Data_t *report = (PCRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumButton)
    {
    case RockBandDrumsA:
        report->a |= m_lastValue;
        break;
    case RockBandDrumsB:
        report->b |= m_lastValue;
        break;
    case RockBandDrumsX:
        report->x |= m_lastValue;
        break;
    case RockBandDrumsY:
        report->y |= m_lastValue;
        break;
    case RockBandDrumsKick1Pedal:
        report->kick1 |= m_lastValue;
        break;
    case RockBandDrumsKick2Pedal:
        report->kick2 |= m_lastValue;
        break;
    case RockBandDrumsBack:
        report->back |= m_lastValue;
        break;
    case RockBandDrumsStart:
        report->start |= m_lastValue;
        break;
    case RockBandDrumsGuide:
        report->guide |= m_lastValue;
        break;
    case RockBandDrumsDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandDrumsDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandDrumsDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandDrumsDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void RockBandDrumsButtonMapping::update_wii(uint8_t *buf)
{
    // not a thing
}
void RockBandDrumsButtonMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void RockBandDrumsButtonMapping::update_ps2(uint8_t *buf)
{
    // Not a thing - drums were always usb here
}

void RockBandDrumsButtonMapping::update_ps3(uint8_t *buf)
{
    PS3RockBandDrums_Data_t *report = (PS3RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumButton)
    {
    case RockBandDrumsA:
        report->a |= m_lastValue;
        break;
    case RockBandDrumsB:
        report->b |= m_lastValue;
        break;
    case RockBandDrumsX:
        report->x |= m_lastValue;
        break;
    case RockBandDrumsY:
        report->y |= m_lastValue;
        break;
    case RockBandDrumsKick1Pedal:
        report->kick1 |= m_lastValue;
        break;
    case RockBandDrumsKick2Pedal:
        report->kick2 |= m_lastValue;
        break;
    case RockBandDrumsBack:
        report->back |= m_lastValue;
        break;
    case RockBandDrumsStart:
        report->start |= m_lastValue;
        break;
    case RockBandDrumsGuide:
        report->guide |= m_lastValue;
        break;
    case RockBandDrumsDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandDrumsDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandDrumsDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandDrumsDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void RockBandDrumsButtonMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandDrums_Data_t *report = (PS4RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumButton)
    {
    case RockBandDrumsA:
        report->a |= m_lastValue;
        break;
    case RockBandDrumsB:
        report->b |= m_lastValue;
        break;
    case RockBandDrumsX:
        report->x |= m_lastValue;
        break;
    case RockBandDrumsY:
        report->y |= m_lastValue;
        break;
    case RockBandDrumsKick1Pedal:
        report->kick1 |= m_lastValue;
        break;
    case RockBandDrumsKick2Pedal:
        report->kick2 |= m_lastValue;
        break;
    case RockBandDrumsBack:
        report->back |= m_lastValue;
        break;
    case RockBandDrumsStart:
        report->start |= m_lastValue;
        break;
    case RockBandDrumsGuide:
        report->guide |= m_lastValue;
        break;
    case RockBandDrumsDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandDrumsDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandDrumsDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandDrumsDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void RockBandDrumsButtonMapping::update_ps5(uint8_t *buf)
{
    PS5RockBandDrums_Data_t *report = (PS5RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumButton)
    {
    case RockBandDrumsA:
        report->a |= m_lastValue;
        break;
    case RockBandDrumsB:
        report->b |= m_lastValue;
        break;
    case RockBandDrumsX:
        report->x |= m_lastValue;
        break;
    case RockBandDrumsY:
        report->y |= m_lastValue;
        break;
    case RockBandDrumsKick1Pedal:
        report->kick1 |= m_lastValue;
        break;
    case RockBandDrumsKick2Pedal:
        report->kick2 |= m_lastValue;
        break;
    case RockBandDrumsBack:
        report->back |= m_lastValue;
        break;
    case RockBandDrumsStart:
        report->start |= m_lastValue;
        break;
    case RockBandDrumsGuide:
        report->guide |= m_lastValue;
        break;
    case RockBandDrumsDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandDrumsDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandDrumsDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandDrumsDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void RockBandDrumsButtonMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumButton)
    {
    case RockBandDrumsA:
        report->a |= m_lastValue;
        break;
    case RockBandDrumsB:
        report->b |= m_lastValue;
        break;
    case RockBandDrumsX:
        report->x |= m_lastValue;
        break;
    case RockBandDrumsY:
        report->y |= m_lastValue;
        break;
    case RockBandDrumsKick1Pedal:
        report->kick1 |= m_lastValue;
        break;
    case RockBandDrumsKick2Pedal:
        report->kick2 |= m_lastValue;
        break;
    case RockBandDrumsBack:
        report->back |= m_lastValue;
        break;
    case RockBandDrumsStart:
        report->start |= m_lastValue;
        break;
    case RockBandDrumsGuide:
        report->guide |= m_lastValue;
        break;
    case RockBandDrumsDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandDrumsDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandDrumsDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandDrumsDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void RockBandDrumsButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandDrums_Data_t *report = (OGXboxRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumButton)
    {
    case RockBandDrumsA:
        report->a |= m_lastValue;
        break;
    case RockBandDrumsB:
        report->b |= m_lastValue;
        break;
    case RockBandDrumsX:
        report->x |= m_lastValue;
        break;
    case RockBandDrumsY:
        report->y |= m_lastValue;
        break;
    case RockBandDrumsKick1Pedal:
        report->kick1 |= m_lastValue;
        break;
    case RockBandDrumsKick2Pedal:
        report->kick2 |= m_lastValue;
        break;
    case RockBandDrumsBack:
        report->back |= m_lastValue;
        break;
    case RockBandDrumsStart:
        report->start |= m_lastValue;
        break;
    case RockBandDrumsDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandDrumsDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandDrumsDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandDrumsDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}