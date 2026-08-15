#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

GuitarHeroArcadeButtonMapping::GuitarHeroArcadeButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void GuitarHeroArcadeButtonMapping::update_hid(uint8_t *buf)
{
    ArcadeGuitarHeroGuitar_Data_t *report = (ArcadeGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghaButton)
    {
    case GuitarHeroArcade_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroArcade_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroArcade_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroArcade_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroArcade_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroArcade_StrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroArcade_StrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroArcade_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroArcade_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    case GuitarHeroArcade_Side:
        report->side |= m_lastValue;
        break;
    }
}
void GuitarHeroArcadeButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
}
void GuitarHeroArcadeButtonMapping::update_switch(uint8_t *buf)
{
}

void GuitarHeroArcadeButtonMapping::update_ps2(uint8_t *buf)
{
}

void GuitarHeroArcadeButtonMapping::update_ps3(uint8_t *buf)
{
}

void GuitarHeroArcadeButtonMapping::update_ps4(uint8_t *buf)
{
}

void GuitarHeroArcadeButtonMapping::update_ps5(uint8_t *buf)
{
}

void GuitarHeroArcadeButtonMapping::update_xinput(uint8_t *buf)
{
}
void GuitarHeroArcadeButtonMapping::update_ogxbox(uint8_t *buf)
{
}
void GuitarHeroArcadeButtonMapping::update_xboxone(uint8_t *buf)
{
}