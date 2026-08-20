#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

ProGuitarButtonMapping::ProGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void ProGuitarButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void ProGuitarButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void ProGuitarButtonMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void ProGuitarButtonMapping::update_ps2(uint8_t *buf)
{
    // not a thing
}

void ProGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3RockBandProGuitar_Data_t *report = (PS3RockBandProGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proButton)
    {
    case ProGuitar_Green:
        report->green |= m_lastValue;
        break;
    case ProGuitar_Red:
        report->red |= m_lastValue;
        break;
    case ProGuitar_Yellow:
        report->yellow |= m_lastValue;
        break;
    case ProGuitar_Blue:
        report->blue |= m_lastValue;
        break;
    case ProGuitar_Orange:
        report->orange |= m_lastValue;
        break;
    case ProGuitar_SoloGreen:
        report->green |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_SoloRed:
        report->red |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_SoloYellow:
        report->yellow |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_SoloBlue:
        report->blue |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_SoloOrange:
        report->orange |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_Pedal:
        report->pedal |= m_lastValue;
        break;
    default:
        break;
    }
}

void ProGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    // not a thing
}

void ProGuitarButtonMapping::update_ps5(uint8_t *buf)
{
    // not a thing
}

void ProGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandProGuitar_Data_t *report = (XInputRockBandProGuitar_Data_t *)buf;
    
    switch (m_mapping.mapping.mapping.proButton)
    {
    case ProGuitar_Green:
        report->green |= m_lastValue;
        break;
    case ProGuitar_Red:
        report->red |= m_lastValue;
        break;
    case ProGuitar_Yellow:
        report->yellow |= m_lastValue;
        break;
    case ProGuitar_Blue:
        report->blue |= m_lastValue;
        break;
    case ProGuitar_Orange:
        report->orange |= m_lastValue;
        break;
    case ProGuitar_SoloGreen:
        report->green |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_SoloRed:
        report->red |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_SoloYellow:
        report->yellow |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_SoloBlue:
        report->blue |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_SoloOrange:
        report->orange |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_Pedal:
        report->pedal |= m_lastValue;
        break;
    default:
        break;
    }
}
void ProGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandProGuitar_Data_t *report = (OGXboxRockBandProGuitar_Data_t *)buf;
    
    switch (m_mapping.mapping.mapping.proButton)
    {
    case ProGuitar_Green:
        report->green |= m_lastValue;
        break;
    case ProGuitar_Red:
        report->red |= m_lastValue;
        break;
    case ProGuitar_Yellow:
        report->yellow |= m_lastValue;
        break;
    case ProGuitar_Blue:
        report->blue |= m_lastValue;
        break;
    case ProGuitar_Orange:
        report->orange |= m_lastValue;
        break;
    case ProGuitar_SoloGreen:
        report->green |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_SoloRed:
        report->red |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_SoloYellow:
        report->yellow |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_SoloBlue:
        report->blue |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitar_SoloOrange:
        report->orange |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    default:
        break;
    }
}
void ProGuitarButtonMapping::update_xboxone(uint8_t *buf)
{
}