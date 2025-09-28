#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

GuitarHeroGuitarAxisMapping::GuitarHeroGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : AxisMapping(mapping, std::move(input), id, mapping.mapping.ghAxis == GuitarHeroGuitarWhammy)
{
}

void GuitarHeroGuitarAxisMapping::update_hid(uint8_t *buf)
{
    PCGuitarHeroGuitar_Data_t *report = (PCGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    }
}
void GuitarHeroGuitarAxisMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats probably
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        // report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarAxisMapping::update_switch(uint8_t *buf)
{
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarAxisMapping::update_ps2(uint8_t *buf)
{
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarAxisMapping::update_ps3(uint8_t *buf)
{
    PS3GuitarHeroGuitar_Data_t *report = (PS3GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    }
}

void GuitarHeroGuitarAxisMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        // shove stick on the slider, then it can be used in menus
        if (m_calibratedValue != 32767)
        {
            report->slider = m_calibratedValue - 32767;
        }
        break;
    case GuitarHeroGuitarLeftStickY:
        report->unused = m_calibratedValue - 32767;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue - 32767;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue - 32767;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGuitarHeroGuitar_Data_t *report = (OGXboxGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        // report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        // report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}