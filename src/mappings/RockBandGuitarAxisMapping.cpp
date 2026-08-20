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
RockBandGuitarAxisMapping::RockBandGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.rbAxis == RockBandGuitar_Whammy)
{
}

void RockBandGuitarAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void RockBandGuitarAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    // TODO: we have to deal with data formats probably
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_LeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Tilt:
        // report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void RockBandGuitarAxisMapping::update_switch(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_LeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void RockBandGuitarAxisMapping::update_ps2(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_LeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void RockBandGuitarAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS3RockBandGuitar_Data_t *report = (PS3RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_LeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = pickupUniversal[m_calibratedValue];
        break;
    case RockBandGuitar_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        report->solo = m_calibratedValue > 65000;
        break;
    case RockBandGuitar_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        report->r2 = m_calibratedValue > 65000;
        break;
    }
}

void RockBandGuitarAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_LeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = m_calibratedValue;
        break;
    case RockBandGuitar_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        report->l2 = m_calibratedValue > 65000;
        break;
    case RockBandGuitar_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        report->r2 = m_calibratedValue > 65000;
        break;
    default:
        break;
    }
}

void RockBandGuitarAxisMapping::update_ps5(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_LeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = m_calibratedValue;
        break;
    case RockBandGuitar_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        report->l2 = m_calibratedValue > 65000;
        break;
    case RockBandGuitar_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        report->r2 = m_calibratedValue > 65000;
        break;
    default:
        break;
    }
}

void RockBandGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputRockBandGuitar_Data_t *report = (XInputRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_LeftStickX:
        report->calibrationSensor = m_calibratedValue - 32768;
        break;
    case RockBandGuitar_LeftStickY:
        report->leftStickY = m_calibratedValue - 32768;
        break;
    case RockBandGuitar_Whammy:
        report->whammy = m_calibratedValue - 32768;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibratedValue - 32768;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = pickupUniversal[m_calibratedValue];
        break;
    case RockBandGuitar_LeftTrigger:
        report->pickup = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_RightTrigger:
        report->unused1 = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void RockBandGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxRockBandGuitar_Data_t *report = (OGXboxRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_LeftStickX:
        report->calibrationSensor = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_LeftStickY:
        report->unused2 = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = pickupUniversal[m_calibratedValue];
        break;
    default:
        break;
    }
}
void RockBandGuitarAxisMapping::update_xboxone(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_LeftStickX:
        report->joystickX = m_calibratedValue - 32768;
        break;
    case RockBandGuitar_LeftStickY:
        report->joystickY = m_calibratedValue - 32768;
        break;
    case RockBandGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = pickupXb1[m_calibratedValue];
        break;
    default:
        break;
    }
}