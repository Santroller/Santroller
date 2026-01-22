#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>
#include <config.hpp>

ProjectDivaAxisMapping::ProjectDivaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.divaAxis == ProjectDivaLeftTrigger || mapping.mapping.divaAxis == ProjectDivaRightTrigger)
{
}

void ProjectDivaAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void ProjectDivaAxisMapping::update_wii(uint8_t *buf)
{
    // TODO: this
}
void ProjectDivaAxisMapping::update_switch(uint8_t *buf)
{
    // https://github.com/ravinrabbid/DivaCon2040
    SwitchGamepad_Data_t *report = (SwitchGamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaAxis)
    {
    case ProjectDivaLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaLeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case ProjectDivaLeftTrigger:
        report->l2 = m_calibratedValue > 60000;
        break;
    case ProjectDivaRightTrigger:
        report->r2 = m_calibratedValue > 60000;
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_ps2(uint8_t *buf)
{
    PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaAxis)
    {
    case ProjectDivaLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaLeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case ProjectDivaLeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_ps3(uint8_t *buf)
{
    if (mode == ModePs3)
    {
        PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)buf;
        switch (m_mapping.mapping.divaAxis)
        {
        case ProjectDivaLeftStickX:
            report->leftStickX = m_calibratedValue >> 8;
            break;
        case ProjectDivaLeftStickY:
            report->leftStickX = m_calibratedValue >> 8;
            break;
        case ProjectDivaRightStickX:
            report->rightStickX = m_calibratedValue >> 8;
            break;
        case ProjectDivaRightStickY:
            report->rightStickY = m_calibratedValue >> 8;
            break;
        case ProjectDivaLeftTrigger:
            report->leftTrigger = m_calibratedValue >> 8;
            break;
        case ProjectDivaRightTrigger:
            report->rightTrigger = m_calibratedValue >> 8;
            break;
        }
        return;
    }
    PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)buf;
    switch (m_mapping.mapping.divaAxis)
    {
    case ProjectDivaLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaLeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case ProjectDivaLeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDivaSlider:
        // todo: this
        break;
    }
}

void ProjectDivaAxisMapping::update_ps4(uint8_t *buf)
{
    PS4Gamepad_Data_t *report = (PS4Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaAxis)
    {
    case ProjectDivaLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaLeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case ProjectDivaLeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_ps5(uint8_t *buf)
{
    PS5Gamepad_Data_t *report = (PS5Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaAxis)
    {
    case ProjectDivaLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaLeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case ProjectDivaLeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_xinput(uint8_t *buf)
{
    XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaAxis)
    {
    case ProjectDivaLeftStickX:
        report->leftStickX = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDivaLeftStickY:
        report->leftStickY = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDivaRightStickX:
        report->rightStickX = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDivaRightStickY:
        report->rightStickY = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDivaLeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void ProjectDivaAxisMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)buf;
    switch (m_mapping.mapping.divaAxis)
    {
    case ProjectDivaLeftStickX:
        report->leftStickX = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDivaLeftStickY:
        report->leftStickY = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDivaRightStickX:
        report->rightStickX = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDivaRightStickY:
        report->rightStickY = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDivaLeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDivaRightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void ProjectDivaAxisMapping::update_xboxone(uint8_t *buf)
{
}