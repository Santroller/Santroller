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

const uint8_t GamepadButtonMapping::dpad_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
GamepadButtonMapping::GamepadButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void GamepadButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void GamepadButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    if (format == 1)
    {
        WiiClassicDataFormat1_t *report = (WiiClassicDataFormat1_t *)buf;
        switch (m_mapping.mapping.mapping.gamepadButton)
        {
        case Gamepad_A:
            report->a |= m_last_value;
            break;
        case Gamepad_B:
            report->b |= m_last_value;
            break;
        case Gamepad_X:
            report->x |= m_last_value;
            break;
        case Gamepad_Y:
            report->y |= m_last_value;
            break;
        case Gamepad_Start:
            report->start |= m_last_value;
            break;
        case Gamepad_Back:
            report->back |= m_last_value;
            break;
        case Gamepad_Capture:
            // report->capture |= m_last_value;
            break;
        case Gamepad_Guide:
            report->guide |= m_last_value;
            break;
        case Gamepad_LeftShoulder:
            report->leftShoulder |= m_last_value;
            break;
        case Gamepad_RightShoulder:
            report->rightShoulder |= m_last_value;
            break;
        case Gamepad_DpadUp:
            report->dpadUp |= m_last_value;
            break;
        case Gamepad_DpadDown:
            report->dpadDown |= m_last_value;
            break;
        case Gamepad_DpadLeft:
            report->dpadLeft |= m_last_value;
            break;
        case Gamepad_DpadRight:
            report->dpadRight |= m_last_value;
            break;
        default:
            break;
        }
    }
    if (format == 2)
    {
        WiiClassicDataFormat2_t *report = (WiiClassicDataFormat2_t *)buf;
        switch (m_mapping.mapping.mapping.gamepadButton)
        {
        case Gamepad_A:
            report->a |= m_last_value;
            break;
        case Gamepad_B:
            report->b |= m_last_value;
            break;
        case Gamepad_X:
            report->x |= m_last_value;
            break;
        case Gamepad_Y:
            report->y |= m_last_value;
            break;
        case Gamepad_Start:
            report->start |= m_last_value;
            break;
        case Gamepad_Back:
            report->back |= m_last_value;
            break;
        case Gamepad_Capture:
            // report->capture |= m_last_value;
            break;
        case Gamepad_Guide:
            report->guide |= m_last_value;
            break;
        case Gamepad_LeftShoulder:
            report->leftShoulder |= m_last_value;
            break;
        case Gamepad_RightShoulder:
            report->rightShoulder |= m_last_value;
            break;
        case Gamepad_DpadUp:
            report->dpadUp |= m_last_value;
            break;
        case Gamepad_DpadDown:
            report->dpadDown |= m_last_value;
            break;
        case Gamepad_DpadLeft:
            report->dpadLeft |= m_last_value;
            break;
        case Gamepad_DpadRight:
            report->dpadRight |= m_last_value;
            break;
        default:
            break;
        }
    }
    if (format == 3)
    {
        WiiClassicDataFormat3_t *report = (WiiClassicDataFormat3_t *)buf;
        switch (m_mapping.mapping.mapping.gamepadButton)
        {
        case Gamepad_A:
            report->a |= m_last_value;
            break;
        case Gamepad_B:
            report->b |= m_last_value;
            break;
        case Gamepad_X:
            report->x |= m_last_value;
            break;
        case Gamepad_Y:
            report->y |= m_last_value;
            break;
        case Gamepad_Start:
            report->start |= m_last_value;
            break;
        case Gamepad_Back:
            report->back |= m_last_value;
            break;
        case Gamepad_Capture:
            // report->capture |= m_last_value;
            break;
        case Gamepad_Guide:
            report->guide |= m_last_value;
            break;
        case Gamepad_LeftShoulder:
            report->leftShoulder |= m_last_value;
            break;
        case Gamepad_RightShoulder:
            report->rightShoulder |= m_last_value;
            break;
        case Gamepad_DpadUp:
            report->dpadUp |= m_last_value;
            break;
        case Gamepad_DpadDown:
            report->dpadDown |= m_last_value;
            break;
        case Gamepad_DpadLeft:
            report->dpadLeft |= m_last_value;
            break;
        case Gamepad_DpadRight:
            report->dpadRight |= m_last_value;
            break;
        default:
            break;
        }
    }
}

void GamepadButtonMapping::update_wiimote_core(wiimote_buttons *buttons)
{
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A: buttons->a |= m_last_value; break;
    case Gamepad_B: buttons->b |= m_last_value; break;
    case Gamepad_Back: buttons->minus |= m_last_value; break;
    case Gamepad_Start: buttons->plus |= m_last_value; break;
    case Gamepad_Guide: buttons->home |= m_last_value; break;
    case Gamepad_DpadUp: buttons->up |= m_last_value; break;
    case Gamepad_DpadDown: buttons->down |= m_last_value; break;
    case Gamepad_DpadLeft: buttons->left |= m_last_value; break;
    case Gamepad_DpadRight: buttons->right |= m_last_value; break;
    default: break;
    }
}

void GamepadButtonMapping::update_switch(uint8_t *buf)
{
    SwitchInputReport *report = (SwitchInputReport *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_last_value;
        break;
    case Gamepad_B:
        report->b |= m_last_value;
        break;
    case Gamepad_X:
        report->x |= m_last_value;
        break;
    case Gamepad_Y:
        report->y |= m_last_value;
        break;
    case Gamepad_Start:
        report->start |= m_last_value;
        break;
    case Gamepad_Back:
        report->back |= m_last_value;
        break;
    case Gamepad_Guide:
        report->guide |= m_last_value;
        break;
    case Gamepad_Capture:
        report->capture |= m_last_value;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_last_value;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_last_value;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_last_value;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_last_value;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_last_value;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_last_value;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_last_value;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_last_value;
        break;
    }
}
void GamepadButtonMapping::update_ps2(uint8_t *buf)
{
    // TODO: pressures
    PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_last_value;
        break;
    case Gamepad_B:
        report->b |= m_last_value;
        break;
    case Gamepad_X:
        report->x |= m_last_value;
        break;
    case Gamepad_Y:
        report->y |= m_last_value;
        break;
    case Gamepad_Start:
        report->start |= m_last_value;
        break;
    case Gamepad_Back:
        report->back |= m_last_value;
        break;
    case Gamepad_Capture:
        // report->capture |= m_last_value;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_last_value;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_last_value;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_last_value;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_last_value;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_last_value;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_last_value;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_last_value;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_last_value;
        break;
    default:
        break;
    }
}
void GamepadButtonMapping::update_ps3(uint8_t *buf)
{
    PS3ThirdPartyGamepad_Data_t *report = (PS3ThirdPartyGamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_last_value;
        break;
    case Gamepad_B:
        report->b |= m_last_value;
        break;
    case Gamepad_X:
        report->x |= m_last_value;
        break;
    case Gamepad_Y:
        report->y |= m_last_value;
        break;
    case Gamepad_Start:
        report->start |= m_last_value;
        break;
    case Gamepad_Back:
        report->back |= m_last_value;
        break;
    case Gamepad_Guide:
        report->guide |= m_last_value;
        break;
    case Gamepad_Capture:
        report->capture |= m_last_value;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_last_value;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_last_value;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_last_value;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_last_value;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_last_value;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_last_value;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_last_value;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_last_value;
        break;
    default:
        break;
    }
}

void GamepadButtonMapping::update_ps4(uint8_t *buf)
{
    PS4Gamepad_Data_t *report = (PS4Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_last_value;
        break;
    case Gamepad_B:
        report->b |= m_last_value;
        break;
    case Gamepad_X:
        report->x |= m_last_value;
        break;
    case Gamepad_Y:
        report->y |= m_last_value;
        break;
    case Gamepad_Start:
        report->start |= m_last_value;
        break;
    case Gamepad_Back:
        report->back |= m_last_value;
        break;
    case Gamepad_Guide:
        report->guide |= m_last_value;
        break;
    case Gamepad_Capture:
        report->capture |= m_last_value;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_last_value;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_last_value;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_last_value;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_last_value;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_last_value;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_last_value;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_last_value;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_last_value;
        break;
    default:
        break;
    }
}
void GamepadButtonMapping::update_ps5(uint8_t *buf)
{
    PS5Gamepad_Data_t *report = (PS5Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_last_value;
        break;
    case Gamepad_B:
        report->b |= m_last_value;
        break;
    case Gamepad_X:
        report->x |= m_last_value;
        break;
    case Gamepad_Y:
        report->y |= m_last_value;
        break;
    case Gamepad_Start:
        report->start |= m_last_value;
        break;
    case Gamepad_Back:
        report->back |= m_last_value;
        break;
    case Gamepad_Guide:
        report->guide |= m_last_value;
        break;
    case Gamepad_Capture:
        report->touchpad |= m_last_value;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_last_value;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_last_value;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_last_value;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_last_value;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_last_value;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_last_value;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_last_value;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_last_value;
        break;
    default:
        break;
    }
}
void GamepadButtonMapping::update_xinput(uint8_t *buf)
{
    XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_last_value;
        break;
    case Gamepad_B:
        report->b |= m_last_value;
        break;
    case Gamepad_X:
        report->x |= m_last_value;
        break;
    case Gamepad_Y:
        report->y |= m_last_value;
        break;
    case Gamepad_Start:
        report->start |= m_last_value;
        break;
    case Gamepad_Back:
        report->back |= m_last_value;
        break;
    case Gamepad_Guide:
        report->guide |= m_last_value;
        break;
    case Gamepad_Capture:
        report->capture |= m_last_value;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_last_value;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_last_value;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_last_value;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_last_value;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_last_value;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_last_value;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_last_value;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_last_value;
        break;
    default:
        break;
    }
}
void GamepadButtonMapping::update_ogxbox(uint8_t *buf)
{
    // TODO: pressures
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_last_value;
        break;
    case Gamepad_B:
        report->b |= m_last_value;
        break;
    case Gamepad_X:
        report->x |= m_last_value;
        break;
    case Gamepad_Y:
        report->y |= m_last_value;
        break;
    case Gamepad_Start:
        report->start |= m_last_value;
        break;
    case Gamepad_Back:
        report->back |= m_last_value;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_last_value;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_last_value;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_last_value;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_last_value;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_last_value;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_last_value;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_last_value;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_last_value;
        break;
    default:
        break;
    }
}
void GamepadButtonMapping::update_xboxone(uint8_t *buf)
{
    XboxOneGamepad_Data_t *report = (XboxOneGamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_last_value;
        break;
    case Gamepad_B:
        report->b |= m_last_value;
        break;
    case Gamepad_X:
        report->x |= m_last_value;
        break;
    case Gamepad_Y:
        report->y |= m_last_value;
        break;
    case Gamepad_Start:
        report->start |= m_last_value;
        break;
    case Gamepad_Back:
        report->back |= m_last_value;
        break;
    case Gamepad_Guide:
        report->guide |= m_last_value;
        break;
    case Gamepad_Capture:
        if (m_last_value)
        {
            report->consoleFunctions[0] = 0x01;
        }
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_last_value;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_last_value;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_last_value;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_last_value;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_last_value;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_last_value;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_last_value;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_last_value;
        break;
    default:
        break;
    }
}
PS3GamepadButtonMapping::PS3GamepadButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadButtonMapping(mapping, std::move(input), id, profile)
{
}
void PS3GamepadButtonMapping::update_ps3(uint8_t *buf)
{
    // TODO: pressures
    PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_last_value;
        break;
    case Gamepad_B:
        report->b |= m_last_value;
        break;
    case Gamepad_X:
        report->x |= m_last_value;
        break;
    case Gamepad_Y:
        report->y |= m_last_value;
        break;
    case Gamepad_Start:
        report->start |= m_last_value;
        break;
    case Gamepad_Back:
        report->back |= m_last_value;
        break;
    case Gamepad_Guide:
        report->guide |= m_last_value;
        break;
    case Gamepad_Capture:
        report->capture |= m_last_value;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_last_value;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_last_value;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_last_value;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_last_value;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_last_value;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_last_value;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_last_value;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_last_value;
        break;
    default:
        break;
    }
    return;
}
