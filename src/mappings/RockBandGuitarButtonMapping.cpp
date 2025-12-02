#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

RockBandGuitarButtonMapping::RockBandGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
    
}

void RockBandGuitarButtonMapping::update_hid(uint8_t *buf)
{
    PCRockBandGuitar_Data_t *report = (PCRockBandGuitar_Data_t *)buf;
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
void RockBandGuitarButtonMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats probably
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case RockBandGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitarTilt:
        // report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void RockBandGuitarButtonMapping::update_switch(uint8_t *buf)
{
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case RockBandGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void RockBandGuitarButtonMapping::update_ps2(uint8_t *buf)
{
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case RockBandGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void RockBandGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3RockBandGuitar_Data_t *report = (PS3RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case RockBandGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    }
}

void RockBandGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case RockBandGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void RockBandGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandGuitar_Data_t *report = (XInputRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case RockBandGuitarLeftStickX:
        // TODO: map to dpad here
        // report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitarLeftStickY:
        // report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void RockBandGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandGuitar_Data_t *report = (OGXboxRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case RockBandGuitarLeftStickX:
        // report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitarLeftStickY:
        // report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}