#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

GuitarHeroArcadeButtonMapping::GuitarHeroArcadeButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
    
}

void GuitarHeroArcadeButtonMapping::update_hid(uint8_t *buf)
{
    ArcadeGuitarHeroGuitar_Data_t *report = (ArcadeGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghaButton)
    {
    case GuitarHeroArcadeGreen:
        report->a |= m_lastValue;
        break;
    case GuitarHeroArcadeRed:
        report->b |= m_lastValue;
        break;
    case GuitarHeroArcadeYellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroArcadeBlue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroArcadeOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroArcadeStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroArcadeStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroArcadeDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroArcadeDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    case GuitarHeroArcadeSide:
        report->side |= m_lastValue;
        break;
    }
}
void GuitarHeroArcadeButtonMapping::update_wii(uint8_t *buf)
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