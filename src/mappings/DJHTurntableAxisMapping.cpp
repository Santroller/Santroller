#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

DJHTurntableAxisMapping::DJHTurntableAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}

void DJHTurntableAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void DJHTurntableAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    WiiTurntableIntermediateFormat3_t intermediate;
    WiiTurntableDataFormat3_t *report = (WiiTurntableDataFormat3_t *)buf;
    switch (m_mapping.mapping.djhAxis)
    {
    case DJHTurntable_LeftStickX:
        report->leftStickX = m_calibratedValue >> 10;
        break;
    case DJHTurntable_LeftStickY:
        report->leftStickY = m_calibratedValue >> 10;
        break;
    case DJHTurntable_LeftVelocity:
        intermediate.leftTableVelocity = m_calibratedValue >> 10;
        report->leftTableVelocity40 = intermediate.leftTableVelocity40;
        report->leftTableVelocity5 = intermediate.leftTableVelocity5;
        break;
    case DJHTurntable_RightVelocity:
        intermediate.rightTableVelocity = m_calibratedValue >> 10;
        report->rightTableVelocity0 = intermediate.rightTableVelocity0;
        report->rightTableVelocity21 = intermediate.rightTableVelocity21;
        report->rightTableVelocity43 = intermediate.rightTableVelocity43;
        report->rightTableVelocity5 = intermediate.rightTableVelocity5;
        break;
    case DJHTurntable_EffectsKnob:
        intermediate.effectsKnob = m_calibratedValue >> 11;
        report->effectsKnob20 = intermediate.effectsKnob20;
        report->effectsKnob43 = intermediate.effectsKnob43;
        break;
    case DJHTurntable_Crossfader:
        report->crossfader = m_calibratedValue >> 12;
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
    if (m_centered)
    {
        return;
    }
    PS3DJHTurntable_Data_t *report = (PS3DJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.djhAxis)
    {
    case DJHTurntable_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case DJHTurntable_LeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case DJHTurntable_LeftVelocity:
        report->leftTableVelocity = m_calibratedValue >> 8;
        break;
    case DJHTurntable_RightVelocity:
        report->rightTableVelocity = m_calibratedValue >> 8;
        break;
    case DJHTurntable_EffectsKnob:
        report->effectsKnob = m_calibratedValue >> 6;
        break;
    case DJHTurntable_Crossfader:
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
    if (m_centered)
    {
        return;
    }
    XInputDJHTurntable_Data_t *report = (XInputDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.djhAxis)
    {
    case DJHTurntable_LeftStickX:
        report->leftTableVelocity = m_calibratedValue - 32768;
        break;
    case DJHTurntable_LeftStickY:
        report->rightTableVelocity = m_calibratedValue - 32768;
        break;
    case DJHTurntable_LeftVelocity:
        report->leftTableVelocity = m_calibratedValue - 32768;
        break;
    case DJHTurntable_RightVelocity:
        report->rightTableVelocity = m_calibratedValue - 32768;
        break;
    case DJHTurntable_EffectsKnob:
        report->effectsKnob = m_calibratedValue - 32768;
        break;
    case DJHTurntable_Crossfader:
        report->crossfader = m_calibratedValue - 32768;
        break;
    default:
        break;
    }
}
void DJHTurntableAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxDJHTurntable_Data_t *report = (OGXboxDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.djhAxis)
    {
    case DJHTurntable_LeftStickX:
        report->leftTableVelocity = m_calibratedValue - 32768;
        break;
    case DJHTurntable_LeftStickY:
        report->rightTableVelocity = m_calibratedValue - 32768;
        break;
    case DJHTurntable_LeftVelocity:
        report->leftTableVelocity = m_calibratedValue - 32768;
        break;
    case DJHTurntable_RightVelocity:
        report->rightTableVelocity = m_calibratedValue - 32768;
        break;
    case DJHTurntable_EffectsKnob:
        report->effectsKnob = m_calibratedValue - 32768;
        break;
    case DJHTurntable_Crossfader:
        report->crossfader = m_calibratedValue - 32768;
        break;
    default:
        break;
    }
}
void DJHTurntableAxisMapping::update_xboxone(uint8_t *buf)
{
    // not a thing
}