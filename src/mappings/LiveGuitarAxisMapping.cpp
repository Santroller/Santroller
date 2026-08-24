#include "mappings/mapping.hpp"
#include "instance.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

LiveGuitarAxisMapping::LiveGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.ghlAxis == GuitarHeroLiveGuitar_Whammy)
{
}

void LiveGuitarAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void LiveGuitarAxisMapping::update_wii(uint8_t format, uint8_t *buf)
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
    if (m_centered)
    {
        return;
    }
    PS3GHLGuitar_Data_t *report = (PS3GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroLiveGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    }
}

void LiveGuitarAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS4GHLGuitar_Data_t *report = (PS4GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroLiveGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void LiveGuitarAxisMapping::update_ps5(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS5GHLGuitar_Data_t *report = (PS5GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroLiveGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void LiveGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputGHLGuitar_Data_t *report = (XInputGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitar_Whammy:
        report->whammy = m_calibratedValue - 32768;
        break;
    case GuitarHeroLiveGuitar_Tilt:
        report->tilt = m_calibratedValue - 32768;
        break;
    }
}
void LiveGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxGHLGuitar_Data_t *report = (OGXboxGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitar_Whammy:
        report->whammy = m_calibratedValue - 32768;
        break;
    case GuitarHeroLiveGuitar_Tilt:
        report->tilt = m_calibratedValue - 32768;
        break;
    }
}
void LiveGuitarAxisMapping::update_xboxone(uint8_t *buf)
{
}