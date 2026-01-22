#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

GuitarHeroGuitarButtonMapping::GuitarHeroGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void GuitarHeroGuitarButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void GuitarHeroGuitarButtonMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats probably
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case GuitarHeroGuitarGreen:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitarRed:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitarYellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitarBlue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitarTapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitarTapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitarTapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitarTapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitarTapOrange:
        report->tapOrange |= m_lastValue;
        break;
    case GuitarHeroGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void GuitarHeroGuitarButtonMapping::update_switch(uint8_t *buf)
{
}

void GuitarHeroGuitarButtonMapping::update_ps2(uint8_t *buf)
{
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case GuitarHeroGuitarGreen:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitarRed:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitarYellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitarBlue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitarTapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitarTapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitarTapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitarTapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitarTapOrange:
        report->tapOrange |= m_lastValue;
        break;
    case GuitarHeroGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3GuitarHeroGuitar_Data_t *report = (PS3GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case GuitarHeroGuitarGreen:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitarRed:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitarYellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitarBlue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitarTapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitarTapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitarTapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitarTapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitarTapOrange:
        report->tapOrange |= m_lastValue;
        break;
    case GuitarHeroGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case GuitarHeroGuitarGreen:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitarRed:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitarYellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitarBlue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitarTapGreen:
        report->soloGreen |= m_lastValue;
        break;
    case GuitarHeroGuitarTapRed:
        report->soloRed |= m_lastValue;
        break;
    case GuitarHeroGuitarTapYellow:
        report->soloYellow |= m_lastValue;
        break;
    case GuitarHeroGuitarTapBlue:
        report->soloBlue |= m_lastValue;
        break;
    case GuitarHeroGuitarTapOrange:
        report->soloOrange |= m_lastValue;
        break;
    case GuitarHeroGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}


void GuitarHeroGuitarButtonMapping::update_ps5(uint8_t *buf)
{
    PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case GuitarHeroGuitarGreen:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitarRed:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitarYellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitarBlue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitarTapGreen:
        report->soloGreen |= m_lastValue;
        break;
    case GuitarHeroGuitarTapRed:
        report->soloRed |= m_lastValue;
        break;
    case GuitarHeroGuitarTapYellow:
        report->soloYellow |= m_lastValue;
        break;
    case GuitarHeroGuitarTapBlue:
        report->soloBlue |= m_lastValue;
        break;
    case GuitarHeroGuitarTapOrange:
        report->soloOrange |= m_lastValue;
        break;
    case GuitarHeroGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case GuitarHeroGuitarGreen:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitarRed:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitarYellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitarBlue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitarTapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitarTapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitarTapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitarTapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitarTapOrange:
        report->tapOrange |= m_lastValue;
        break;
    case GuitarHeroGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void GuitarHeroGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGuitarHeroGuitar_Data_t *report = (OGXboxGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case GuitarHeroGuitarGreen:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitarRed:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitarYellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitarBlue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitarTapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitarTapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitarTapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitarTapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitarTapOrange:
        report->tapOrange |= m_lastValue;
        break;
    case GuitarHeroGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void GuitarHeroGuitarButtonMapping::update_xboxone(uint8_t *buf)
{
}