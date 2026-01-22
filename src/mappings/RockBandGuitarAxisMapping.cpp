#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>
static uint8_t pickupUniversal[] = {0x19, 0x4c, 0x96, 0xb2, 0xe5};
static uint8_t pickupXb1[] = {0x00, 0x10, 0x20, 0x30, 0x40};
RockBandGuitarAxisMapping::RockBandGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.rbAxis == RockBandGuitarWhammy)
{
}

void RockBandGuitarAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void RockBandGuitarAxisMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats probably
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)buf;
    switch (m_mapping.mapping.rbAxis)
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
void RockBandGuitarAxisMapping::update_switch(uint8_t *buf)
{
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.rbAxis)
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

void RockBandGuitarAxisMapping::update_ps2(uint8_t *buf)
{
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.rbAxis)
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

void RockBandGuitarAxisMapping::update_ps3(uint8_t *buf)
{
    PS3RockBandGuitar_Data_t *report = (PS3RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.rbAxis)
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
    case RockBandGuitarPickup:
        report->pickup = pickupUniversal[m_lastValue];
        break;
    }
}

void RockBandGuitarAxisMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.rbAxis)
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
    case RockBandGuitarPickup:
        report->pickup = pickupUniversal[m_lastValue];
        break;
    default:
        break;
    }
}

void RockBandGuitarAxisMapping::update_ps5(uint8_t *buf)
{
    PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.rbAxis)
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
    case RockBandGuitarPickup:
        report->pickup = pickupUniversal[m_lastValue];
        break;
    default:
        break;
    }
}

void RockBandGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandGuitar_Data_t *report = (XInputRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.rbAxis)
    {
    case RockBandGuitarLeftStickX:
        report->calibrationSensor = m_calibratedValue - 32767;
        break;
    case RockBandGuitarLeftStickY:
        report->leftStickY = m_calibratedValue - 32767;
        break;
    case RockBandGuitarWhammy:
        report->whammy = m_calibratedValue - 32767;
        break;
    case RockBandGuitarTilt:
        report->tilt = m_calibratedValue - 32767;
        break;
    case RockBandGuitarPickup:
        report->pickup = pickupUniversal[m_lastValue];
        break;
    default:
        break;
    }
}
void RockBandGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandGuitar_Data_t *report = (OGXboxRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.rbAxis)
    {
    case RockBandGuitarLeftStickX:
        report->calibrationSensor = m_calibratedValue >> 8;
        break;
    case RockBandGuitarLeftStickY:
        report->unused2 = m_calibratedValue >> 8;
        break;
    case RockBandGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    case RockBandGuitarPickup:
        report->pickup = pickupUniversal[m_lastValue];
        break;
    default:
        break;
    }
}
void RockBandGuitarAxisMapping::update_xboxone(uint8_t *buf)
{
}