#include "mappings/mapping.hpp"
#include "instance.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

GuitarHeroDrumsAxisMapping::GuitarHeroDrumsAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void GuitarHeroDrumsAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void GuitarHeroDrumsAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    // TODO: this one is a bit fun because we can only send one velocity at a time.
    // TODO: deal with stick now that gamepad isnt here
    WiiDrumDataFormat3_t *report = (WiiDrumDataFormat3_t *)buf;
    switch (m_mapping.mapping.mapping.ghDrumAxis)
    {
    case GuitarHeroDrums_RedPad:
        report->b = true;
        break;
    case GuitarHeroDrums_YellowPad:
        report->y = true;
        break;
    case GuitarHeroDrums_BluePad:
        report->x = true;
        break;
    case GuitarHeroDrums_OrangePad:
        report->rightShoulder = true;
        break;
    case GuitarHeroDrums_GreenPad:
        report->a = true;
        break;
    case GuitarHeroDrums_KickPedal:
        report->leftShoulder = true;
        break;
    // case GuitarHeroDrums_LeftStickX:
    //     report->leftStickX = m_calibratedValue >> 10;
    //     break;
    // case GuitarHeroDrums_LeftStickY:
    //     report->leftStickY = m_calibratedValue >> 10;
    //     break;
    default:
        break;
    }
}
void GuitarHeroDrumsAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing on switch
}

void GuitarHeroDrumsAxisMapping::update_ps2(uint8_t *buf)
{
    // not a thing on ps2
}

void GuitarHeroDrumsAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS3GuitarHeroDrums_Data_t *report = (PS3GuitarHeroDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghDrumAxis)
    {
    case GuitarHeroDrums_RedPad:
        report->redVelocity = m_calibratedValue >> 8;
        report->b = true;
        break;
    case GuitarHeroDrums_YellowPad:
        report->yellowVelocity = m_calibratedValue >> 8;
        report->y = true;
        break;
    case GuitarHeroDrums_BluePad:
        report->blueVelocity = m_calibratedValue >> 8;
        report->x = true;
        break;
    case GuitarHeroDrums_OrangePad:
        report->orangeVelocity = m_calibratedValue >> 8;
        report->rightShoulder = true;
        break;
    case GuitarHeroDrums_GreenPad:
        report->greenVelocity = m_calibratedValue >> 8;
        report->a = true;
        break;
    case GuitarHeroDrums_KickPedal:
        report->kickVelocity = m_calibratedValue >> 8;
        report->leftShoulder = true;
        break;
    }
}

void GuitarHeroDrumsAxisMapping::update_ps4(uint8_t *buf)
{
    // not a thing on ps4
}

void GuitarHeroDrumsAxisMapping::update_ps5(uint8_t *buf)
{
    // not a thing on ps4
}

void GuitarHeroDrumsAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputGuitarHeroDrums_Data_t *report = (XInputGuitarHeroDrums_Data_t *)buf;

    switch (m_mapping.mapping.mapping.ghDrumAxis)
    {
    case GuitarHeroDrums_RedPad:
        report->redVelocity = m_calibratedValue - 32768;
        report->b = true;
        break;
    case GuitarHeroDrums_YellowPad:
        report->yellowVelocity = m_calibratedValue - 32768;
        report->y = true;
        break;
    case GuitarHeroDrums_BluePad:
        report->blueVelocity = m_calibratedValue - 32768;
        report->x = true;
        break;
    case GuitarHeroDrums_OrangePad:
        report->orangeVelocity = m_calibratedValue - 32768;
        report->rightShoulder = true;
        break;
    case GuitarHeroDrums_GreenPad:
        report->greenVelocity = m_calibratedValue - 32768;
        report->a = true;
        break;
    case GuitarHeroDrums_KickPedal:
        report->kickVelocity = m_calibratedValue - 32768;
        report->leftShoulder = true;
        break;
    }
}
void GuitarHeroDrumsAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxGuitarHeroDrums_Data_t *report = (OGXboxGuitarHeroDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghDrumAxis)
    {
    case GuitarHeroDrums_RedPad:
        report->redVelocity = m_calibratedValue - 32768;
        report->b = true;
        break;
    case GuitarHeroDrums_YellowPad:
        report->yellowVelocity = m_calibratedValue - 32768;
        report->y = true;
        break;
    case GuitarHeroDrums_BluePad:
        report->blueVelocity = m_calibratedValue - 32768;
        report->x = true;
        break;
    case GuitarHeroDrums_OrangePad:
        report->orangeVelocity = m_calibratedValue - 32768;
        report->rightShoulder = true;
        break;
    case GuitarHeroDrums_GreenPad:
        report->greenVelocity = m_calibratedValue - 32768;
        report->a = true;
        break;
    case GuitarHeroDrums_KickPedal:
        report->kickVelocity = m_calibratedValue - 32768;
        report->leftShoulder = true;
        break;
    }
}
void GuitarHeroDrumsAxisMapping::update_xboxone(uint8_t *buf)
{
}