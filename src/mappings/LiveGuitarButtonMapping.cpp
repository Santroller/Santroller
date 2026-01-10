#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

LiveGuitarButtonMapping::LiveGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
}

void LiveGuitarButtonMapping::update_hid(uint8_t *buf)
{
    PCGHLGuitar_Data_t *report = (PCGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case GuitarHeroLiveGuitarWhite1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarWhite2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarWhite3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = INT16_MAX;
        break;
    case GuitarHeroLiveGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = INT16_MIN;
        break;
    case GuitarHeroLiveGuitarDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void LiveGuitarButtonMapping::update_wii(uint8_t *buf)
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
    case GuitarHeroLiveGuitarWhite1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarWhite2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarWhite3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = 0;
        break;
    case GuitarHeroLiveGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = 0xFF;
        break;
    case GuitarHeroLiveGuitarDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void LiveGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    PS4GHLGuitar_Data_t *report = (PS4GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitarWhite1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarWhite2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarWhite3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = 0;
        break;
    case GuitarHeroLiveGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = 0xFF;
        break;
    case GuitarHeroLiveGuitarDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void LiveGuitarButtonMapping::update_ps5(uint8_t *buf)
{
    PS5GHLGuitar_Data_t *report = (PS5GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitarWhite1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarWhite2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarWhite3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = 0;
        break;
    case GuitarHeroLiveGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = 0xFF;
        break;
    case GuitarHeroLiveGuitarDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void LiveGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputGHLGuitar_Data_t *report = (XInputGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitarWhite1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarWhite2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarWhite3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = INT16_MAX;
        break;
    case GuitarHeroLiveGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = INT16_MIN;
        break;
    case GuitarHeroLiveGuitarDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void LiveGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGHLGuitar_Data_t *report = (OGXboxGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitarWhite1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarWhite2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarWhite3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBlack3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarBack:
        report->back |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarStart:
        report->start |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = INT16_MAX;
        break;
    case GuitarHeroLiveGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = INT16_MIN;
        break;
    case GuitarHeroLiveGuitarDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case GuitarHeroLiveGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}