#include "mappings/mapping.hpp"
#include "instance.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>
static uint8_t pickupUniversal[] = {0x19, 0x4c, 0x96, 0xb2, 0xe5};
static uint8_t pickupXb1[] = {0x00, 0x10, 0x20, 0x30, 0x40};
PowerGigGuitarAxisMapping::PowerGigGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.rbAxis == PowerGigGuitarWhammy)
{
}

void PowerGigGuitarAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void PowerGigGuitarAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    // TODO: we have to deal with data formats probably
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case PowerGigGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarTilt:
        // report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void PowerGigGuitarAxisMapping::update_switch(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case PowerGigGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void PowerGigGuitarAxisMapping::update_ps2(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case PowerGigGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void PowerGigGuitarAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS3PowerGigGuitar_Data_t *report = (PS3PowerGigGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case PowerGigGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarPickup:
        report->pickup = pickupUniversal[m_lastValue];
        break;
    }
}

void PowerGigGuitarAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS4PowerGigGuitar_Data_t *report = (PS4PowerGigGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case PowerGigGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarPickup:
        report->pickup = pickupUniversal[m_lastValue];
        break;
    default:
        break;
    }
}

void PowerGigGuitarAxisMapping::update_ps5(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS5PowerGigGuitar_Data_t *report = (PS5PowerGigGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case PowerGigGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarPickup:
        report->pickup = pickupUniversal[m_lastValue];
        break;
    default:
        break;
    }
}

void PowerGigGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputPowerGigGuitar_Data_t *report = (XInputPowerGigGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case PowerGigGuitarLeftStickX:
        report->calibrationSensor = m_calibratedValue - 32768;
        break;
    case PowerGigGuitarLeftStickY:
        report->leftStickY = m_calibratedValue - 32768;
        break;
    case PowerGigGuitarWhammy:
        report->whammy = m_calibratedValue - 32768;
        break;
    case PowerGigGuitarTilt:
        report->tilt = m_calibratedValue - 32768;
        break;
    case PowerGigGuitarPickup:
        report->pickup = pickupUniversal[m_lastValue];
        break;
    default:
        break;
    }
}
void PowerGigGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxPowerGigGuitar_Data_t *report = (OGXboxPowerGigGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case PowerGigGuitarLeftStickX:
        report->calibrationSensor = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarLeftStickY:
        report->unused2 = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    case PowerGigGuitarPickup:
        report->pickup = pickupUniversal[m_lastValue];
        break;
    default:
        break;
    }
}
void PowerGigGuitarAxisMapping::update_xboxone(uint8_t *buf)
{
}