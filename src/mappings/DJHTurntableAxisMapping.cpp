#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

DJHTurntableAxisMapping::DJHTurntableAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : AxisMapping(mapping, std::move(input), id, false)
{
}

void DJHTurntableAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void DJHTurntableAxisMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats probably
    WiiTurntableIntermediateFormat3_t *report = (WiiTurntableIntermediateFormat3_t *)buf;
    switch (m_mapping.mapping.djhAxis)
    {
    case DJHTurntableLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case DJHTurntableLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case DJHTurntableLeftVelocity:
        report->leftTableVelocity = m_calibratedValue >> 8;
        break;
    case DJHTurntableRightVelocity:
        report->rightTableVelocity = m_calibratedValue >> 8;
        break;
    case DJHTurntableEffectsKnob:
        report->effectsKnob = m_calibratedValue - 32767;
        break;
    case DJHTurntableCrossfader:
        report->crossfader = m_calibratedValue - 32767;
        break;
    default:
        break;
    }
}
void DJHTurntableAxisMapping::update_switch(uint8_t *buf)
{
    // no switch turntable
}

void DJHTurntableAxisMapping::update_ps2(uint8_t *buf)
{
    // no ps2 turntable
}

void DJHTurntableAxisMapping::update_ps3(uint8_t *buf)
{
    PS3DJHTurntable_Data_t *report = (PS3DJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.djhAxis)
    {
    case DJHTurntableLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case DJHTurntableLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case DJHTurntableLeftVelocity:
        report->leftTableVelocity = m_calibratedValue >> 8;
        break;
    case DJHTurntableRightVelocity:
        report->rightTableVelocity = m_calibratedValue >> 8;
        break;
    case DJHTurntableEffectsKnob:
        report->effectsKnob = m_calibratedValue >> 6;
        break;
    case DJHTurntableCrossfader:
        report->crossfader = m_calibratedValue >> 6;
        break;
    }
}

void DJHTurntableAxisMapping::update_ps4(uint8_t *buf)
{
    // no ps4 turntable
}

void DJHTurntableAxisMapping::update_ps5(uint8_t *buf)
{
    // no ps5 turntable
}

void DJHTurntableAxisMapping::update_xinput(uint8_t *buf)
{
    XInputDJHTurntable_Data_t *report = (XInputDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.djhAxis)
    {
    case DJHTurntableLeftStickX:
        report->leftTableVelocity = m_calibratedValue - 32767;
        break;
    case DJHTurntableLeftStickY:
        report->rightTableVelocity = m_calibratedValue - 32767;
        break;
    case DJHTurntableLeftVelocity:
        report->leftTableVelocity = m_calibratedValue - 32767;
        break;
    case DJHTurntableRightVelocity:
        report->rightTableVelocity = m_calibratedValue - 32767;
        break;
    case DJHTurntableEffectsKnob:
        report->effectsKnob = m_calibratedValue - 32767;
        break;
    case DJHTurntableCrossfader:
        report->crossfader = m_calibratedValue - 32767;
        break;
    default:
        break;
    }
}
void DJHTurntableAxisMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxDJHTurntable_Data_t *report = (OGXboxDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.djhAxis)
    {
    case DJHTurntableLeftStickX:
        report->leftTableVelocity = m_calibratedValue - 32767;
        break;
    case DJHTurntableLeftStickY:
        report->rightTableVelocity = m_calibratedValue - 32767;
        break;
    case DJHTurntableLeftVelocity:
        report->leftTableVelocity = m_calibratedValue - 32767;
        break;
    case DJHTurntableRightVelocity:
        report->rightTableVelocity = m_calibratedValue - 32767;
        break;
    case DJHTurntableEffectsKnob:
        report->effectsKnob = m_calibratedValue - 32767;
        break;
    case DJHTurntableCrossfader:
        report->crossfader = m_calibratedValue - 32767;
        break;
    default:
        break;
    }
}