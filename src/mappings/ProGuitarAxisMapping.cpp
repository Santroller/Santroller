#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

ProGuitarAxisMapping::ProGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : AxisMapping(mapping, std::move(input), id, mapping.mapping.proAxis != ProGuitarLeftStickX && mapping.mapping.proAxis != ProGuitarLeftStickY && mapping.mapping.proAxis != ProGuitarTilt)
{
}

void ProGuitarAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void ProGuitarAxisMapping::update_wii(uint8_t *buf)
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
    PS3RockBandProGuitar_Data_t *report = (PS3RockBandProGuitar_Data_t *)buf;
    switch (m_mapping.mapping.proAxis)
    {
    case ProGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case ProGuitarLowEFret:
        report->lowEFret = m_calibratedValue >> 8;
        break;
    case ProGuitarAFret:
        report->aFret = m_calibratedValue >> 8;
        break;
    case ProGuitarDFret:
        report->dFret = m_calibratedValue >> 8;
        break;
    case ProGuitarGFret:
        report->gFret = m_calibratedValue >> 8;
        break;
    case ProGuitarBFret:
        report->bFret = m_calibratedValue >> 8;
        break;
    case ProGuitarHighEFret:
        report->highEFret = m_calibratedValue >> 8;
        break;
    case ProGuitarLowEFretVelocity:
        report->lowEFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarAFretVelocity:
        report->aFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarDFretVelocity:
        report->dFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarGFretVelocity:
        report->gFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarBFretVelocity:
        report->bFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarHighEFretVelocity:
        report->highEFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
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
    XInputRockBandProGuitar_Data_t *report = (XInputRockBandProGuitar_Data_t *)buf;
    switch (m_mapping.mapping.proAxis)
    {
    case ProGuitarLeftStickX:
        report->leftStickX = m_calibratedValue - 32767;
        break;
    case ProGuitarLeftStickY:
        report->leftStickY = m_calibratedValue - 32767;
        break;
    case ProGuitarLowEFret:
        report->lowEFret = m_calibratedValue >> 8;
        break;
    case ProGuitarAFret:
        report->aFret = m_calibratedValue >> 8;
        break;
    case ProGuitarDFret:
        report->dFret = m_calibratedValue >> 8;
        break;
    case ProGuitarGFret:
        report->gFret = m_calibratedValue >> 8;
        break;
    case ProGuitarBFret:
        report->bFret = m_calibratedValue >> 8;
        break;
    case ProGuitarHighEFret:
        report->highEFret = m_calibratedValue >> 8;
        break;
    case ProGuitarLowEFretVelocity:
        report->lowEFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarAFretVelocity:
        report->aFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarDFretVelocity:
        report->dFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarGFretVelocity:
        report->gFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarBFretVelocity:
        report->bFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarHighEFretVelocity:
        report->highEFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    }
}
void ProGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandProGuitar_Data_t *report = (OGXboxRockBandProGuitar_Data_t *)buf;
    switch (m_mapping.mapping.proAxis)
    {
    case ProGuitarLeftStickX:
        report->leftStickX = m_calibratedValue - 32767;
        break;
    case ProGuitarLeftStickY:
        report->leftStickY = m_calibratedValue - 32767;
        break;
    case ProGuitarLowEFret:
        report->lowEFret = m_calibratedValue >> 8;
        break;
    case ProGuitarAFret:
        report->aFret = m_calibratedValue >> 8;
        break;
    case ProGuitarDFret:
        report->dFret = m_calibratedValue >> 8;
        break;
    case ProGuitarGFret:
        report->gFret = m_calibratedValue >> 8;
        break;
    case ProGuitarBFret:
        report->bFret = m_calibratedValue >> 8;
        break;
    case ProGuitarHighEFret:
        report->highEFret = m_calibratedValue >> 8;
        break;
    case ProGuitarLowEFretVelocity:
        report->lowEFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarAFretVelocity:
        report->aFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarDFretVelocity:
        report->dFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarGFretVelocity:
        report->gFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarBFretVelocity:
        report->bFretVelocity = m_calibratedValue >> 8;
        break;
    case ProGuitarHighEFretVelocity:
        report->highEFretVelocity = m_calibratedValue >> 8;
        break;
    }
}