#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

const uint8_t GuitarHeroGuitarAxisMapping::gh5_slider_mapping[] = {
    0x80, 0x15, 0x4D, 0x30, 0x9A, 0x99, 0x66,
    0x65, 0xC9, 0xC7, 0xC8, 0xC6, 0xAF, 0xAD,
    0xAE, 0xAC, 0xFF, 0xFB, 0xFD, 0xF9, 0xFE,
    0xFA, 0xFC, 0xF8, 0xE6, 0xE2, 0xE4, 0xE0,
    0xE5, 0xE1, 0xE3, 0xDF};

GuitarHeroGuitarAxisMapping::GuitarHeroGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.ghAxis == GuitarHeroGuitarWhammy)
{
}

void GuitarHeroGuitarAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
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
void GuitarHeroGuitarAxisMapping::update_ps5(uint8_t *buf)
{
    PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)buf;
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
        if (!m_centered)
        {
            report->slider = m_calibratedValue - 32767;
        }
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue - 32767;
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
        // shove stick on the slider, then it can be used in menus
        if (!m_centered)
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
    }
}