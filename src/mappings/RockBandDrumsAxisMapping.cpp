#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

RockBandDrumsAxisMapping::RockBandDrumsAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : AxisMapping(mapping, std::move(input), id, mapping.mapping.rbDrumAxis != RockBandDrumsLeftStickX && mapping.mapping.rbDrumAxis != RockBandDrumsLeftStickY)
{
}

void RockBandDrumsAxisMapping::update_hid(uint8_t *buf)
{
    PCRockBandDrums_Data_t *report = (PCRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumAxis)
    {
    case RockBandDrumsRedPad:
        report->redVelocity = m_calibratedValue - 32767;
        report->b = true;
        break;
    case RockBandDrumsYellowPad:
        report->yellowVelocity = m_calibratedValue - 32767;
        report->y = true;
        break;
    case RockBandDrumsBluePad:
        report->blueVelocity = m_calibratedValue - 32767;
        report->x = true;
        break;
    case RockBandDrumsGreenPad:
        report->greenVelocity = m_calibratedValue - 32767;
        report->a = true;
        break;
    case RockBandDrumsLeftStickX:
        if (!m_centered)
        {
            report->redVelocity = m_calibratedValue - 32767;
        }
        break;
    case RockBandDrumsLeftStickY:
        if (!m_centered)
        {
            report->yellowVelocity = m_calibratedValue - 32767;
        }
        break;
    }
}
void RockBandDrumsAxisMapping::update_wii(uint8_t *buf)
{
    // not a thing
}
void RockBandDrumsAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing on switch
}

void RockBandDrumsAxisMapping::update_ps2(uint8_t *buf)
{
    // not a thing on ps2
}

void RockBandDrumsAxisMapping::update_ps3(uint8_t *buf)
{
    PS3RockBandDrums_Data_t *report = (PS3RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumAxis)
    {
    case RockBandDrumsRedPad:
        report->redVelocity = m_calibratedValue >> 8;
        report->b = true;
        break;
    case RockBandDrumsYellowPad:
        report->yellowVelocity = m_calibratedValue >> 8;
        report->y = true;
        break;
    case RockBandDrumsBluePad:
        report->blueVelocity = m_calibratedValue >> 8;
        report->x = true;
        break;
    case RockBandDrumsGreenPad:
        report->greenVelocity = m_calibratedValue >> 8;
        report->a = true;
        break;
    case RockBandDrumsLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case RockBandDrumsLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    }
}

void RockBandDrumsAxisMapping::update_ps4(uint8_t *buf)
{
    // not a thing on ps4
}

void RockBandDrumsAxisMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)buf;

    switch (m_mapping.mapping.rbDrumAxis)
    {
    case RockBandDrumsRedPad:
        report->redVelocity = m_calibratedValue - 32767;
        report->b = true;
        break;
    case RockBandDrumsYellowPad:
        report->yellowVelocity = m_calibratedValue - 32767;
        report->y = true;
        break;
    case RockBandDrumsBluePad:
        report->blueVelocity = m_calibratedValue - 32767;
        report->x = true;
        break;
    case RockBandDrumsGreenPad:
        report->greenVelocity = m_calibratedValue - 32767;
        report->a = true;
        break;
    case RockBandDrumsLeftStickX:
        if (!m_centered)
        {
            report->redVelocity = m_calibratedValue - 32767;
        }
        break;
    case RockBandDrumsLeftStickY:
        if (!m_centered)
        {
            report->yellowVelocity = m_calibratedValue - 32767;
        }
        break;
    }
}
void RockBandDrumsAxisMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandDrums_Data_t *report = (OGXboxRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumAxis)
    {
    case RockBandDrumsRedPad:
        report->redVelocity = m_calibratedValue - 32767;
        report->b = true;
        break;
    case RockBandDrumsYellowPad:
        report->yellowVelocity = m_calibratedValue - 32767;
        report->y = true;
        break;
    case RockBandDrumsBluePad:
        report->blueVelocity = m_calibratedValue - 32767;
        report->x = true;
        break;
    case RockBandDrumsGreenPad:
        report->greenVelocity = m_calibratedValue - 32767;
        report->a = true;
        break;
    case RockBandDrumsLeftStickX:
        if (!m_centered)
        {
            report->redVelocity = m_calibratedValue - 32767;
        }
        break;
    case RockBandDrumsLeftStickY:
        if (!m_centered)
        {
            report->yellowVelocity = m_calibratedValue - 32767;
        }
        break;
    }
}