#include "mappings/mapping.hpp"
#include "instance.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

GuitarHeroGuitarButtonMapping::GuitarHeroGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void GuitarHeroGuitarButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void GuitarHeroGuitarButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->tapOrange |= m_lastValue;
        break;
    }
}
void GuitarHeroGuitarButtonMapping::update_switch(uint8_t *buf)
{
    SwitchInputReport *report = (SwitchInputReport *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps2(uint8_t *buf)
{
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        // TODO: get a ps2 wt guitar pcb and see if we can figure this one out
        // report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->tapOrange |= m_lastValue;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3GuitarHeroGuitar_Data_t *report = (PS3GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->tapOrange |= m_lastValue;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->soloGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->soloRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->soloYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->soloBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->soloOrange |= m_lastValue;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps5(uint8_t *buf)
{
    PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->soloGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->soloRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->soloYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->soloBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->soloOrange |= m_lastValue;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->tapOrange |= m_lastValue;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGuitarHeroGuitar_Data_t *report = (OGXboxGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->tapOrange |= m_lastValue;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarButtonMapping::update_xboxone(uint8_t *buf)
{
    XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->soloGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->soloRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->soloYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->soloBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->soloOrange |= m_lastValue;
        break;
    default:
        break;
    }
}