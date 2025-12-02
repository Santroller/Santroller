#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

LiveGuitarAxisMapping::LiveGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : AxisMapping(mapping, std::move(input), id, mapping.mapping.ghlAxis == GuitarHeroLiveGuitarWhammy)
{
}

void LiveGuitarAxisMapping::update_hid(uint8_t *buf)
{
    PCGHLGuitar_Data_t *report = (PCGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitarLeftStickX:
        report->leftStickX = m_calibratedValue - 32767;
        break;
    case GuitarHeroLiveGuitarLeftStickY:
        if (!m_centered)
        {
            report->strumBar = m_calibratedValue - 32767;
        }
        break;
    case GuitarHeroLiveGuitarWhammy:
        report->whammy = m_calibratedValue - 32767;
        break;
    case GuitarHeroLiveGuitarTilt:
        report->tilt = m_calibratedValue - 32767;
        break;
    }
}
void LiveGuitarAxisMapping::update_wii(uint8_t *buf)
{
    // not a thing
}
void LiveGuitarAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void LiveGuitarAxisMapping::update_ps2(uint8_t *buf)
{
    // not a thing
}

void LiveGuitarAxisMapping::update_ps3(uint8_t *buf)
{
    PS3GHLGuitar_Data_t *report = (PS3GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroLiveGuitarLeftStickY:
        if (!m_centered)
        {
            report->strumBar = m_calibratedValue >> 8;
        }
        break;
    case GuitarHeroLiveGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroLiveGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    }
}

void LiveGuitarAxisMapping::update_ps4(uint8_t *buf)
{
    PS4GHLGuitar_Data_t *report = (PS4GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroLiveGuitarLeftStickY:
        if (!m_centered)
        {
            report->strumBar = m_calibratedValue >> 8;
        }
        break;
    case GuitarHeroLiveGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroLiveGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void LiveGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    XInputGHLGuitar_Data_t *report = (XInputGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitarLeftStickX:
        report->leftStickX = m_calibratedValue - 32767;
        break;
    case GuitarHeroLiveGuitarLeftStickY:
        if (!m_centered)
        {
            report->strumBar = m_calibratedValue - 32767;
        }
        break;
    case GuitarHeroLiveGuitarWhammy:
        report->whammy = m_calibratedValue - 32767;
        break;
    case GuitarHeroLiveGuitarTilt:
        report->tilt = m_calibratedValue - 32767;
        break;
    }
}
void LiveGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGHLGuitar_Data_t *report = (OGXboxGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitarLeftStickX:
        report->leftStickX = m_calibratedValue - 32767;
        break;
    case GuitarHeroLiveGuitarLeftStickY:
        if (!m_centered)
        {
            report->strumBar = m_calibratedValue - 32767;
        }
        break;
    case GuitarHeroLiveGuitarWhammy:
        report->whammy = m_calibratedValue - 32767;
        break;
    case GuitarHeroLiveGuitarTilt:
        report->tilt = m_calibratedValue - 32767;
        break;
    }
}