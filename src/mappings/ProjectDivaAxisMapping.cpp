#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>
#include <config.hpp>

ProjectDivaAxisMapping::ProjectDivaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.divaAxis == ProjectDiva_LeftTrigger || mapping.mapping.mapping.divaAxis == ProjectDiva_RightTrigger)
{
}

void ProjectDivaAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void ProjectDivaAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // TODO: this
}
void ProjectDivaAxisMapping::update_switch(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    // https://github.com/ravinrabbid/DivaCon2040
    SwitchGamepad_Data_t *report = (SwitchGamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_LeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case ProjectDiva_LeftTrigger:
        report->l2 = m_calibratedValue > 60000;
        break;
    case ProjectDiva_RightTrigger:
        report->r2 = m_calibratedValue > 60000;
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_ps2(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_LeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case ProjectDiva_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    if (mode == ModePs3)
    {
        PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)buf;
        switch (m_mapping.mapping.mapping.divaAxis)
        {
        case ProjectDiva_LeftStickX:
            report->leftStickX = m_calibratedValue >> 8;
            break;
        case ProjectDiva_LeftStickY:
            report->leftStickX = m_calibratedValue >> 8;
            break;
        case ProjectDiva_RightStickX:
            report->rightStickX = m_calibratedValue >> 8;
            break;
        case ProjectDiva_RightStickY:
            report->rightStickY = m_calibratedValue >> 8;
            break;
        case ProjectDiva_LeftTrigger:
            report->leftTrigger = m_calibratedValue >> 8;
            break;
        case ProjectDiva_RightTrigger:
            report->rightTrigger = m_calibratedValue >> 8;
            break;
        case ProjectDiva_Slider:
            break;
        }
        return;
    }
    PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_LeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case ProjectDiva_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDiva_Slider:
        // todo: this
        break;
    }
}

void ProjectDivaAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS4Gamepad_Data_t *report = (PS4Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_LeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case ProjectDiva_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_ps5(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS5Gamepad_Data_t *report = (PS5Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_LeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_LeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case ProjectDiva_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_LeftStickX:
        report->leftStickX = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDiva_LeftStickY:
        report->leftStickY = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDiva_RightStickX:
        report->rightStickX = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDiva_RightStickY:
        report->rightStickY = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDiva_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void ProjectDivaAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_LeftStickX:
        report->leftStickX = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDiva_LeftStickY:
        report->leftStickY = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDiva_RightStickX:
        report->rightStickX = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDiva_RightStickY:
        report->rightStickY = m_calibratedValue - INT16_MAX;
        break;
    case ProjectDiva_LeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case ProjectDiva_RightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void ProjectDivaAxisMapping::update_xboxone(uint8_t *buf)
{
}