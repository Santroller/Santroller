#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

LiveGuitarButtonMapping::LiveGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void LiveGuitarButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void LiveGuitarButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void LiveGuitarButtonMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void LiveGuitarButtonMapping::update_ps2(uint8_t *buf)
{
    // not a thing
}

void LiveGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3GHLGuitar_Data_t *report = (PS3GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitar_White1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Back:
        report->back |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Start:
        report->start |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Guide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = 0;
        break;
    case GuitarHeroLiveGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = 0xFF;
        break;
    case GuitarHeroLiveGuitar_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}

void LiveGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    PS4GHLGuitar_Data_t *report = (PS4GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitar_White1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Back:
        report->back |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Start:
        report->start |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Guide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = 0;
        break;
    case GuitarHeroLiveGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = 0xFF;
        break;
    case GuitarHeroLiveGuitar_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}

void LiveGuitarButtonMapping::update_ps5(uint8_t *buf)
{
    PS5GHLGuitar_Data_t *report = (PS5GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitar_White1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Back:
        report->back |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Start:
        report->start |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Guide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = 0;
        break;
    case GuitarHeroLiveGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = 0xFF;
        break;
    case GuitarHeroLiveGuitar_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}

void LiveGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputGHLGuitar_Data_t *report = (XInputGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitar_White1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Back:
        report->back |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Start:
        report->start |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Guide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = INT16_MAX;
        break;
    case GuitarHeroLiveGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = INT16_MIN;
        break;
    case GuitarHeroLiveGuitar_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}
void LiveGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGHLGuitar_Data_t *report = (OGXboxGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitar_White1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Back:
        report->back |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Start:
        report->start |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = INT16_MAX;
        break;
    case GuitarHeroLiveGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = INT16_MIN;
        break;
    case GuitarHeroLiveGuitar_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}
void LiveGuitarButtonMapping::update_xboxone(uint8_t *buf)
{
}