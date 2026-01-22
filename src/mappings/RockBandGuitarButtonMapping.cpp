#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

RockBandGuitarButtonMapping::RockBandGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void RockBandGuitarButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void RockBandGuitarButtonMapping::update_wii(uint8_t *buf)
{
   // not a thing, was hid
}
void RockBandGuitarButtonMapping::update_switch(uint8_t *buf)
{
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case RockBandGuitarGreen:
        report->a |= m_lastValue;
        break;
    case RockBandGuitarRed:
        report->b |= m_lastValue;
        break;
    case RockBandGuitarYellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitarBlue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitarSoloGreen:
        report->a |= m_lastValue;
        break;
    case RockBandGuitarSoloRed:
        report->b |= m_lastValue;
        break;
    case RockBandGuitarSoloYellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitarSoloBlue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitarSoloOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitarBack:
        report->back |= m_lastValue;
        break;
    case RockBandGuitarStart:
        report->start |= m_lastValue;
        break;
    case RockBandGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case RockBandGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void RockBandGuitarButtonMapping::update_ps2(uint8_t *buf)
{
    // was hid
}

void RockBandGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3RockBandGuitar_Data_t *report = (PS3RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case RockBandGuitarGreen:
        report->a |= m_lastValue;
        break;
    case RockBandGuitarRed:
        report->b |= m_lastValue;
        break;
    case RockBandGuitarYellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitarBlue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitarSoloGreen:
        report->a |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloRed:
        report->b |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloYellow:
        report->y |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloBlue:
        report->x |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloOrange:
        report->leftShoulder |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarBack:
        report->back |= m_lastValue;
        break;
    case RockBandGuitarStart:
        report->start |= m_lastValue;
        break;
    case RockBandGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case RockBandGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void RockBandGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case RockBandGuitarGreen:
        report->a |= m_lastValue;
        break;
    case RockBandGuitarRed:
        report->b |= m_lastValue;
        break;
    case RockBandGuitarYellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitarBlue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitarSoloGreen:
        report->a |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloRed:
        report->b |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloYellow:
        report->y |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloBlue:
        report->x |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloOrange:
        report->leftShoulder |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarBack:
        report->back |= m_lastValue;
        break;
    case RockBandGuitarStart:
        report->start |= m_lastValue;
        break;
    case RockBandGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case RockBandGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void RockBandGuitarButtonMapping::update_ps5(uint8_t *buf)
{
    PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case RockBandGuitarGreen:
        report->a |= m_lastValue;
        break;
    case RockBandGuitarRed:
        report->b |= m_lastValue;
        break;
    case RockBandGuitarYellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitarBlue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitarSoloGreen:
        report->a |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloRed:
        report->b |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloYellow:
        report->y |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloBlue:
        report->x |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloOrange:
        report->leftShoulder |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarBack:
        report->back |= m_lastValue;
        break;
    case RockBandGuitarStart:
        report->start |= m_lastValue;
        break;
    case RockBandGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case RockBandGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void RockBandGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandGuitar_Data_t *report = (XInputRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case RockBandGuitarGreen:
        report->a |= m_lastValue;
        break;
    case RockBandGuitarRed:
        report->b |= m_lastValue;
        break;
    case RockBandGuitarYellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitarBlue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitarSoloGreen:
        report->a |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloRed:
        report->b |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloYellow:
        report->y |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloBlue:
        report->x |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloOrange:
        report->leftShoulder |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarBack:
        report->back |= m_lastValue;
        break;
    case RockBandGuitarStart:
        report->start |= m_lastValue;
        break;
    case RockBandGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case RockBandGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void RockBandGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandGuitar_Data_t *report = (OGXboxRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case RockBandGuitarGreen:
        report->a |= m_lastValue;
        break;
    case RockBandGuitarRed:
        report->b |= m_lastValue;
        break;
    case RockBandGuitarYellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitarBlue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitarOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitarSoloGreen:
        report->a |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloRed:
        report->b |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloYellow:
        report->y |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloBlue:
        report->x |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarSoloOrange:
        report->leftShoulder |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitarBack:
        report->back |= m_lastValue;
        break;
    case RockBandGuitarStart:
        report->start |= m_lastValue;
        break;
    case RockBandGuitarGuide:
        break;
    case RockBandGuitarStrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitarStrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void RockBandGuitarButtonMapping::update_xboxone(uint8_t *buf)
{
}