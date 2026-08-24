#include "mappings/mapping.hpp"
#include "instance.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

ProGuitarAxisMapping::ProGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.proAxis != ProGuitar_Tilt)
{
}

void ProGuitarAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void ProGuitarAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void ProGuitarAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void ProGuitarAxisMapping::update_ps2(uint8_t *buf)
{
    // not a thing
}

void ProGuitarAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS3RockBandProGuitar_Data_t *report = (PS3RockBandProGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proAxis)
    {
    case ProGuitar_LowEFret:
        report->lowEFret = m_calibratedValue;
        break;
    case ProGuitar_AFret:
        report->aFret = m_calibratedValue;
        break;
    case ProGuitar_DFret:
        report->dFret = m_calibratedValue;
        break;
    case ProGuitar_GFret:
        report->gFret = m_calibratedValue;
        break;
    case ProGuitar_BFret:
        report->bFret = m_calibratedValue;
        break;
    case ProGuitar_HighEFret:
        report->highEFret = m_calibratedValue;
        break;
    case ProGuitar_LowEFretVelocity:
        report->lowEFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_AFretVelocity:
        report->aFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_DFretVelocity:
        report->dFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_GFretVelocity:
        report->gFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_BFretVelocity:
        report->bFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_HighEFretVelocity:
        report->highEFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void ProGuitarAxisMapping::update_ps4(uint8_t *buf)
{
}

void ProGuitarAxisMapping::update_ps5(uint8_t *buf)
{
}

void ProGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputRockBandProGuitar_Data_t *report = (XInputRockBandProGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proAxis)
    {
    case ProGuitar_LowEFret:
        report->lowEFret = m_calibratedValue;
        break;
    case ProGuitar_AFret:
        report->aFret = m_calibratedValue;
        break;
    case ProGuitar_DFret:
        report->dFret = m_calibratedValue;
        break;
    case ProGuitar_GFret:
        report->gFret = m_calibratedValue;
        break;
    case ProGuitar_BFret:
        report->bFret = m_calibratedValue;
        break;
    case ProGuitar_HighEFret:
        report->highEFret = m_calibratedValue;
        break;
    case ProGuitar_LowEFretVelocity:
        report->lowEFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_AFretVelocity:
        report->aFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_DFretVelocity:
        report->dFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_GFretVelocity:
        report->gFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_BFretVelocity:
        report->bFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_HighEFretVelocity:
        report->highEFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void ProGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxRockBandProGuitar_Data_t *report = (OGXboxRockBandProGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proAxis)
    {
    case ProGuitar_LowEFret:
        report->lowEFret = m_calibratedValue >> 8;
        break;
    case ProGuitar_AFret:
        report->aFret = m_calibratedValue >> 8;
        break;
    case ProGuitar_DFret:
        report->dFret = m_calibratedValue >> 8;
        break;
    case ProGuitar_GFret:
        report->gFret = m_calibratedValue >> 8;
        break;
    case ProGuitar_BFret:
        report->bFret = m_calibratedValue >> 8;
        break;
    case ProGuitar_HighEFret:
        report->highEFret = m_calibratedValue >> 8;
        break;
    case ProGuitar_LowEFretVelocity:
        report->lowEFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_AFretVelocity:
        report->aFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_DFretVelocity:
        report->dFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_GFretVelocity:
        report->gFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_BFretVelocity:
        report->bFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitar_HighEFretVelocity:
        report->highEFretVelocity = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void ProGuitarAxisMapping::update_xboxone(uint8_t *buf)
{
}