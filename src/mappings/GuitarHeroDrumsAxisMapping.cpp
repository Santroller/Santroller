#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

GuitarHeroDrumsAxisMapping::GuitarHeroDrumsAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : AxisMapping(mapping, std::move(input), id, mapping.mapping.ghDrumAxis != GuitarHeroDrumsLeftStickX && mapping.mapping.ghDrumAxis != GuitarHeroDrumsLeftStickY)
{
}

void GuitarHeroDrumsAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void GuitarHeroDrumsAxisMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats probably
    // TODO: this one is a bit fun because we can only send one velocity at a time.
    WiiDrumDataFormat3_t *report = (WiiDrumDataFormat3_t *)buf;
    switch (m_mapping.mapping.ghDrumAxis)
    {
    case GuitarHeroDrumsRedPad:
        report->b = true;
        break;
    case GuitarHeroDrumsYellowPad:
        report->y = true;
        break;
    case GuitarHeroDrumsBluePad:
        report->x = true;
        break;
    case GuitarHeroDrumsOrangePad:
        report->rightShoulder = true;
        break;
    case GuitarHeroDrumsGreenPad:
        report->a = true;
        break;
    case GuitarHeroDrumsKickPedal:
        report->leftShoulder = true;
        break;
    case GuitarHeroDrumsLeftStickX:
        report->leftStickX = m_calibratedValue >> 10;
        break;
    case GuitarHeroDrumsLeftStickY:
        report->leftStickY = m_calibratedValue >> 10;
        break;
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
    PS3GuitarHeroDrums_Data_t *report = (PS3GuitarHeroDrums_Data_t *)buf;
    switch (m_mapping.mapping.ghDrumAxis)
    {
    case GuitarHeroDrumsRedPad:
        report->redVelocity = m_calibratedValue >> 8;
        report->b = true;
        break;
    case GuitarHeroDrumsYellowPad:
        report->yellowVelocity = m_calibratedValue >> 8;
        report->y = true;
        break;
    case GuitarHeroDrumsBluePad:
        report->blueVelocity = m_calibratedValue >> 8;
        report->x = true;
        break;
    case GuitarHeroDrumsOrangePad:
        report->orangeVelocity = m_calibratedValue >> 8;
        report->rightShoulder = true;
        break;
    case GuitarHeroDrumsGreenPad:
        report->greenVelocity = m_calibratedValue >> 8;
        report->a = true;
        break;
    case GuitarHeroDrumsKickPedal:
        report->kickVelocity = m_calibratedValue >> 8;
        report->leftShoulder = true;
        break;
    case GuitarHeroDrumsLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroDrumsLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
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
    XInputGuitarHeroDrums_Data_t *report = (XInputGuitarHeroDrums_Data_t *)buf;

    switch (m_mapping.mapping.ghDrumAxis)
    {
    case GuitarHeroDrumsRedPad:
        report->redVelocity = m_calibratedValue - 32767;
        report->b = true;
        break;
    case GuitarHeroDrumsYellowPad:
        report->yellowVelocity = m_calibratedValue - 32767;
        report->y = true;
        break;
    case GuitarHeroDrumsBluePad:
        report->blueVelocity = m_calibratedValue - 32767;
        report->x = true;
        break;
    case GuitarHeroDrumsOrangePad:
        report->orangeVelocity = m_calibratedValue - 32767;
        report->rightShoulder = true;
        break;
    case GuitarHeroDrumsGreenPad:
        report->greenVelocity = m_calibratedValue - 32767;
        report->a = true;
        break;
    case GuitarHeroDrumsKickPedal:
        report->kickVelocity = m_calibratedValue - 32767;
        report->leftShoulder = true;
        break;
    case GuitarHeroDrumsLeftStickX:
        report->leftStickX = m_calibratedValue - 32767;
        break;
    case GuitarHeroDrumsLeftStickY:
        if (!m_centered)
        {
            report->leftStickY = m_calibratedValue - 32767;
        }
        break;
    }
}
void GuitarHeroDrumsAxisMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGuitarHeroDrums_Data_t *report = (OGXboxGuitarHeroDrums_Data_t *)buf;
    switch (m_mapping.mapping.ghDrumAxis)
    {
    case GuitarHeroDrumsRedPad:
        report->redVelocity = m_calibratedValue - 32767;
        report->b = true;
        break;
    case GuitarHeroDrumsYellowPad:
        report->yellowVelocity = m_calibratedValue - 32767;
        report->y = true;
        break;
    case GuitarHeroDrumsBluePad:
        report->blueVelocity = m_calibratedValue - 32767;
        report->x = true;
        break;
    case GuitarHeroDrumsOrangePad:
        report->orangeVelocity = m_calibratedValue - 32767;
        report->rightShoulder = true;
        break;
    case GuitarHeroDrumsGreenPad:
        report->greenVelocity = m_calibratedValue - 32767;
        report->a = true;
        break;
    case GuitarHeroDrumsKickPedal:
        report->kickVelocity = m_calibratedValue - 32767;
        report->leftShoulder = true;
        break;
    case GuitarHeroDrumsLeftStickX:
        report->leftStickX = m_calibratedValue - 32767;
        break;
    case GuitarHeroDrumsLeftStickY:
        if (!m_centered)
        {
            report->leftStickY = m_calibratedValue - 32767;
        }
        break;
    }
}