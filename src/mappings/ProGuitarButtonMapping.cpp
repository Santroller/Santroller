#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

ProGuitarButtonMapping::ProGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
}

void ProGuitarButtonMapping::update_hid(uint8_t *buf)
{
    PCRockBandProGuitar_Data_t *report = (PCRockBandProGuitar_Data_t *)buf;
    switch (m_mapping.mapping.proButton)
    {
    case ProGuitarA:
        report->a |= m_lastValue;
        break;
    case ProGuitarB:
        report->b |= m_lastValue;
        break;
    case ProGuitarY:
        report->y |= m_lastValue;
        break;
    case ProGuitarX:
        report->x |= m_lastValue;
        break;
    case ProGuitarGreen:
        report->green |= m_lastValue;
        break;
    case ProGuitarRed:
        report->red |= m_lastValue;
        break;
    case ProGuitarYellow:
        report->yellow |= m_lastValue;
        break;
    case ProGuitarBlue:
        report->blue |= m_lastValue;
        break;
    case ProGuitarOrange:
        report->orange |= m_lastValue;
        break;
    case ProGuitarSoloGreen:
        report->green |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloRed:
        report->red |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloYellow:
        report->yellow |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloBlue:
        report->blue |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloOrange:
        report->orange |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarPedal:
        report->pedal |= m_lastValue;
        break;
    case ProGuitarBack:
        report->back |= m_lastValue;
        break;
    case ProGuitarStart:
        report->start |= m_lastValue;
        break;
    case ProGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case ProGuitarDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case ProGuitarDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case ProGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case ProGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void ProGuitarButtonMapping::update_wii(uint8_t *buf)
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
    switch (m_mapping.mapping.proButton)
    {
    case ProGuitarA:
        report->a |= m_lastValue;
        break;
    case ProGuitarB:
        report->b |= m_lastValue;
        break;
    case ProGuitarY:
        report->y |= m_lastValue;
        break;
    case ProGuitarX:
        report->x |= m_lastValue;
        break;
    case ProGuitarGreen:
        report->green |= m_lastValue;
        break;
    case ProGuitarRed:
        report->red |= m_lastValue;
        break;
    case ProGuitarYellow:
        report->yellow |= m_lastValue;
        break;
    case ProGuitarBlue:
        report->blue |= m_lastValue;
        break;
    case ProGuitarOrange:
        report->orange |= m_lastValue;
        break;
    case ProGuitarSoloGreen:
        report->green |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloRed:
        report->red |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloYellow:
        report->yellow |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloBlue:
        report->blue |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloOrange:
        report->orange |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarPedal:
        report->pedal |= m_lastValue;
        break;
    case ProGuitarBack:
        report->back |= m_lastValue;
        break;
    case ProGuitarStart:
        report->start |= m_lastValue;
        break;
    case ProGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case ProGuitarDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case ProGuitarDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case ProGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case ProGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void ProGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    // not a thing
}

void ProGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandProGuitar_Data_t *report = (XInputRockBandProGuitar_Data_t *)buf;
    
    switch (m_mapping.mapping.proButton)
    {
    case ProGuitarA:
        report->a |= m_lastValue;
        break;
    case ProGuitarB:
        report->b |= m_lastValue;
        break;
    case ProGuitarY:
        report->y |= m_lastValue;
        break;
    case ProGuitarX:
        report->x |= m_lastValue;
        break;
    case ProGuitarGreen:
        report->green |= m_lastValue;
        break;
    case ProGuitarRed:
        report->red |= m_lastValue;
        break;
    case ProGuitarYellow:
        report->yellow |= m_lastValue;
        break;
    case ProGuitarBlue:
        report->blue |= m_lastValue;
        break;
    case ProGuitarOrange:
        report->orange |= m_lastValue;
        break;
    case ProGuitarSoloGreen:
        report->green |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloRed:
        report->red |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloYellow:
        report->yellow |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloBlue:
        report->blue |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloOrange:
        report->orange |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarPedal:
        report->pedal |= m_lastValue;
        break;
    case ProGuitarBack:
        report->back |= m_lastValue;
        break;
    case ProGuitarStart:
        report->start |= m_lastValue;
        break;
    case ProGuitarGuide:
        report->guide |= m_lastValue;
        break;
    case ProGuitarDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case ProGuitarDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case ProGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case ProGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void ProGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandProGuitar_Data_t *report = (OGXboxRockBandProGuitar_Data_t *)buf;
    
    switch (m_mapping.mapping.proButton)
    {
    case ProGuitarA:
        report->a |= m_lastValue;
        break;
    case ProGuitarB:
        report->b |= m_lastValue;
        break;
    case ProGuitarY:
        report->y |= m_lastValue;
        break;
    case ProGuitarX:
        report->x |= m_lastValue;
        break;
    case ProGuitarGreen:
        report->green |= m_lastValue;
        break;
    case ProGuitarRed:
        report->red |= m_lastValue;
        break;
    case ProGuitarYellow:
        report->yellow |= m_lastValue;
        break;
    case ProGuitarBlue:
        report->blue |= m_lastValue;
        break;
    case ProGuitarOrange:
        report->orange |= m_lastValue;
        break;
    case ProGuitarSoloGreen:
        report->green |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloRed:
        report->red |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloYellow:
        report->yellow |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloBlue:
        report->blue |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarSoloOrange:
        report->orange |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case ProGuitarBack:
        report->back |= m_lastValue;
        break;
    case ProGuitarStart:
        report->start |= m_lastValue;
        break;
    case ProGuitarDpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case ProGuitarDpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case ProGuitarDpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case ProGuitarDpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}