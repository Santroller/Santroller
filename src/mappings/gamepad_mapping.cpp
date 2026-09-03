#include "mappings/mapping.hpp"
#include "instance.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>
#include <config/config.hpp>

GamepadAxisMapping::GamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.gamepadAxis == Gamepad_LeftTrigger || mapping.mapping.mapping.gamepadAxis == Gamepad_RightTrigger)
{
}

void GamepadAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    update_xinput(buf);
    // and while its almost correct, the y axis is inverted on 360, so we do need to flip that back
    PCGamepadDpad_Data_t *data = (PCGamepadDpad_Data_t *)buf;
    data->leftStickY = -data->leftStickY;
    data->rightStickY = -data->rightStickY;
}
void GamepadAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    if (format == 1)
    {
        WiiIntermediateClassicDataFormat_t test;
        WiiClassicDataFormat1_t *report = (WiiClassicDataFormat1_t *)buf;
        switch (m_mapping.mapping.mapping.gamepadAxis)
        {
        case Gamepad_LeftStickX:
            report->leftStickX = m_calibrated_value >> 10;
            break;
        case Gamepad_LeftStickY:
            report->leftStickY = m_calibrated_value >> 10;
            break;
        case Gamepad_RightStickX:
            test.rightStickX = m_calibrated_value >> 11;
            report->rightStickX0 = test.rightStickX0;
            report->rightStickX21 = test.rightStickX21;
            report->rightStickX43 = test.rightStickX43;
            break;
        case Gamepad_RightStickY:
            report->rightStickY = m_calibrated_value >> 11;
            break;
        case Gamepad_LeftTrigger:
            test.leftTrigger = m_calibrated_value >> 11;
            report->leftTrigger20 = test.leftTrigger20;
            report->leftTrigger43 = test.leftTrigger43;
            break;
        case Gamepad_RightTrigger:
            report->rightTrigger = m_calibrated_value >> 11;
            break;
        default:
            break;
        }
    }
    if (format == 2)
    {
        WiiIntermediateClassicDataFormat2_t test;
        WiiClassicDataFormat2_t *report = (WiiClassicDataFormat2_t *)buf;
        switch (m_mapping.mapping.mapping.gamepadAxis)
        {
        case Gamepad_LeftStickX:
            test.leftStickX = m_calibrated_value >> 6;
            report->leftStickX10 = test.leftStickX10;
            report->leftStickX92 = test.leftStickX92;
            break;
        case Gamepad_LeftStickY:
            test.leftStickY = m_calibrated_value >> 6;
            report->leftStickY10 = test.leftStickY10;
            report->leftStickY92 = test.leftStickY92;
            break;
        case Gamepad_RightStickX:
            test.rightStickX = m_calibrated_value >> 6;
            report->rightStickX10 = test.rightStickX10;
            report->rightStickX92 = test.rightStickX92;
            break;
        case Gamepad_RightStickY:
            test.rightStickY = m_calibrated_value >> 6;
            report->rightStickY10 = test.rightStickY10;
            report->rightStickY92 = test.rightStickY92;
            break;
        case Gamepad_LeftTrigger:
            report->leftTrigger = m_calibrated_value >> 8;
            break;
        case Gamepad_RightTrigger:
            report->rightTrigger = m_calibrated_value >> 8;
            break;
        default:
            break;
        }
    }
    if (format == 3)
    {
        WiiClassicDataFormat3_t *report = (WiiClassicDataFormat3_t *)buf;
        switch (m_mapping.mapping.mapping.gamepadAxis)
        {
        case Gamepad_LeftStickX:
            report->leftStickX = m_calibrated_value >> 8;
            break;
        case Gamepad_LeftStickY:
            report->leftStickY = m_calibrated_value >> 8;
            break;
        case Gamepad_RightStickX:
            report->rightStickX = m_calibrated_value >> 8;
            break;
        case Gamepad_RightStickY:
            report->rightStickY = m_calibrated_value >> 8;
            break;
        case Gamepad_LeftTrigger:
            report->leftTrigger = m_calibrated_value >> 8;
            break;
        case Gamepad_RightTrigger:
            report->rightTrigger = m_calibrated_value >> 8;
            break;
        default:
            break;
        }
    }
}
void GamepadAxisMapping::update_switch(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    SwitchInputReport *report = (SwitchInputReport *)buf;
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        report->leftStickX = m_calibrated_value >> 4;
        break;
    case Gamepad_LeftStickY:
        report->leftStickY = m_calibrated_value >> 4;
        break;
    case Gamepad_RightStickX:
        report->rightStickX = m_calibrated_value >> 4;
        break;
    case Gamepad_RightStickY:
        report->rightStickY = m_calibrated_value >> 4;
        break;
    case Gamepad_LeftTrigger:
        report->leftTrigger = m_calibrated_value > 60000;
        break;
    case Gamepad_RightTrigger:
        report->rightTrigger = m_calibrated_value > 60000;
        break;
    default:
        break;
    }
}

void GamepadAxisMapping::update_ps2(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        report->leftStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_LeftStickY:
        report->leftStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_RightStickX:
        report->rightStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_RightStickY:
        report->rightStickY = m_calibrated_value >> 8;
        break;
    case Gamepad_LeftTrigger:
        report->leftTrigger = m_calibrated_value >> 8;
        break;
    case Gamepad_RightTrigger:
        report->rightTrigger = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}

void GamepadAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        report->leftStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_LeftStickY:
        report->leftStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_RightStickX:
        report->rightStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_RightStickY:
        report->rightStickY = m_calibrated_value >> 8;
        break;
    case Gamepad_LeftTrigger:
        report->leftTrigger = m_calibrated_value >> 8;
        break;
    case Gamepad_RightTrigger:
        report->rightTrigger = m_calibrated_value >> 8;
        break;
    case Gamepad_AccelX:
        report->accelX = m_calibrated_value;
        break;
    case Gamepad_AccelY:
        report->accelY = m_calibrated_value;
        break;
    case Gamepad_AccelZ:
        report->accelZ = m_calibrated_value;
        break;
    case Gamepad_Gyro:
        report->gyro = m_calibrated_value;
        break;
    }
}

void GamepadAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS4Gamepad_Data_t *report = (PS4Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        report->leftStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_LeftStickY:
        report->leftStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_RightStickX:
        report->rightStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_RightStickY:
        report->rightStickY = m_calibrated_value >> 8;
        break;
    case Gamepad_LeftTrigger:
        report->leftTrigger = m_calibrated_value >> 8;
        break;
    case Gamepad_RightTrigger:
        report->rightTrigger = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}

void GamepadAxisMapping::update_ps5(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS5Gamepad_Data_t *report = (PS5Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        report->leftStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_LeftStickY:
        report->leftStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_RightStickX:
        report->rightStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_RightStickY:
        report->rightStickY = m_calibrated_value >> 8;
        break;
    case Gamepad_LeftTrigger:
        report->leftTrigger = m_calibrated_value >> 8;
        break;
    case Gamepad_RightTrigger:
        report->rightTrigger = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}

void GamepadAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        report->leftStickX = m_calibrated_value - 32768;
        break;
    case Gamepad_LeftStickY:
        report->leftStickY = m_calibrated_value - 32768;
        break;
    case Gamepad_RightStickX:
        report->rightStickX = m_calibrated_value - 32768;
        break;
    case Gamepad_RightStickY:
        report->rightStickY = m_calibrated_value - 32768;
        break;
    case Gamepad_LeftTrigger:
        report->leftTrigger = m_calibrated_value >> 8;
        break;
    case Gamepad_RightTrigger:
        report->rightTrigger = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}
void GamepadAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        report->leftStickX = m_calibrated_value - INT16_MAX;
        break;
    case Gamepad_LeftStickY:
        report->leftStickY = m_calibrated_value - INT16_MAX;
        break;
    case Gamepad_RightStickX:
        report->rightStickX = m_calibrated_value - INT16_MAX;
        break;
    case Gamepad_RightStickY:
        report->rightStickY = m_calibrated_value - INT16_MAX;
        break;
    case Gamepad_LeftTrigger:
        report->leftTrigger = m_calibrated_value >> 8;
        break;
    case Gamepad_RightTrigger:
        report->rightTrigger = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}
void GamepadAxisMapping::update_xboxone(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XboxOneGamepad_Data_t *report = (XboxOneGamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        report->leftStickX = m_calibrated_value - 32768;
        break;
    case Gamepad_LeftStickY:
        report->leftStickY = m_calibrated_value - 32768;
        break;
    case Gamepad_RightStickX:
        report->rightStickX = m_calibrated_value - 32768;
        break;
    case Gamepad_RightStickY:
        report->rightStickY = m_calibrated_value - 32768;
        break;
    case Gamepad_LeftTrigger:
        report->leftTrigger = m_calibrated_value >> 6;
        break;
    case Gamepad_RightTrigger:
        report->rightTrigger = m_calibrated_value >> 6;
        break;
    default:
        break;
    }
}
PS3GamepadAxisMapping::PS3GamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadAxisMapping(mapping, std::move(input), id, profile)
{
}

void PS3GamepadAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        report->leftStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_LeftStickY:
        report->leftStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_RightStickX:
        report->rightStickX = m_calibrated_value >> 8;
        break;
    case Gamepad_RightStickY:
        report->rightStickY = m_calibrated_value >> 8;
        break;
    case Gamepad_LeftTrigger:
        report->leftTrigger = m_calibrated_value >> 8;
        break;
    case Gamepad_RightTrigger:
        report->rightTrigger = m_calibrated_value >> 8;
        break;
    case Gamepad_AccelX:
        report->accelX = m_calibrated_value;
        break;
    case Gamepad_AccelY:
        report->accelY = m_calibrated_value;
        break;
    case Gamepad_AccelZ:
        report->accelZ = m_calibrated_value;
        break;
    case Gamepad_Gyro:
        report->gyro = m_calibrated_value;
        break;
    }
    return;
}
