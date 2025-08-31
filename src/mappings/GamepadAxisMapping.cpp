#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

GamepadAxisMapping::GamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : Mapping(id), m_mapping(mapping), m_input(std::move(input)), m_trigger(mapping.mapping.gamepadAxis == GamepadLeftTrigger || mapping.mapping.gamepadAxis == GamepadRightTrigger)
{
}


void GamepadAxisMapping::update(bool full_poll)
{
    auto val = m_input->tickAnalog();
    if (val != m_lastValue || full_poll)
    {
        m_lastValue = val;
        m_calibratedValue = calibrate(val, m_mapping.max, m_mapping.min, m_mapping.deadzone, m_mapping.center, m_trigger);
        proto_Event event = {which_event : proto_Event_axis_tag, event : {axis : {m_id, m_lastValue, m_calibratedValue}}};
        send_event(event);
    }
}
void GamepadAxisMapping::update_hid(uint8_t *buf)
{
    PCGamepad_Data_t *report = (PCGamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadAxis)
    {
    case GamepadLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GamepadLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GamepadRightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case GamepadRightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case GamepadLeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case GamepadRightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    case GamepadAccelX:
        report->accelX = m_lastValue;
        break;
    case GamepadAccelY:
        report->accelY = m_lastValue;
        break;
    case GamepadAccelZ:
        report->accelZ = m_lastValue;
        break;
    case GamepadGyro:
        report->gyro = m_lastValue;
        break;
    }
}
void GamepadAxisMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats.
    WiiClassicDataFormat3_t *report = (WiiClassicDataFormat3_t *)buf;
    switch (m_mapping.mapping.gamepadAxis)
    {
    case GamepadLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GamepadLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GamepadRightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case GamepadRightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case GamepadLeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case GamepadRightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void GamepadAxisMapping::update_switch(uint8_t *buf)
{
    SwitchGamepad_Data_t *report = (SwitchGamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadAxis)
    {
    case GamepadLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GamepadLeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GamepadRightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case GamepadRightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case GamepadLeftTrigger:
        report->l2 = m_lastValue > 60000;
        break;
    case GamepadRightTrigger:
        report->r2 = m_lastValue > 60000;
        break;
    default:
        break;
    }
}

void GamepadAxisMapping::update_ps2(uint8_t *buf)
{
    PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadAxis)
    {
    case GamepadLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GamepadLeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GamepadRightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case GamepadRightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case GamepadLeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case GamepadRightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GamepadAxisMapping::update_ps3(uint8_t *buf)
{
    PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadAxis)
    {
    case GamepadLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GamepadLeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GamepadRightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case GamepadRightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case GamepadLeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case GamepadRightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    case GamepadAccelX:
        report->accelX = m_lastValue;
        break;
    case GamepadAccelY:
        report->accelY = m_lastValue;
        break;
    case GamepadAccelZ:
        report->accelZ = m_lastValue;
        break;
    case GamepadGyro:
        report->gyro = m_lastValue;
        break;
    }
}

void GamepadAxisMapping::update_ps4(uint8_t *buf)
{
    PS4Gamepad_Data_t *report = (PS4Gamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadAxis)
    {
    case GamepadLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GamepadLeftStickY:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GamepadRightStickX:
        report->rightStickX = m_calibratedValue >> 8;
        break;
    case GamepadRightStickY:
        report->rightStickY = m_calibratedValue >> 8;
        break;
    case GamepadLeftTrigger:
        report->leftTrigger = m_calibratedValue >> 8;
        break;
    case GamepadRightTrigger:
        report->rightTrigger = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GamepadAxisMapping::update_xinput(uint8_t *buf)
{
    XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadAxis)
    {
    case GamepadLeftStickX:
        report->leftStickX = m_lastValue;
        break;
    case GamepadLeftStickY:
        report->leftStickY = m_lastValue;
        break;
    case GamepadRightStickX:
        report->rightStickX = m_lastValue;
        break;
    case GamepadRightStickY:
        report->rightStickY = m_lastValue;
        break;
    case GamepadLeftTrigger:
        report->leftTrigger = m_lastValue;
        break;
    case GamepadRightTrigger:
        report->rightTrigger = m_lastValue;
        break;
    default:
        break;
    }
}
void GamepadAxisMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)buf;
    switch (m_mapping.mapping.gamepadAxis)
    {
    case GamepadLeftStickX:
        report->leftStickX = m_lastValue;
        break;
    case GamepadLeftStickY:
        report->leftStickY = m_lastValue;
        break;
    case GamepadRightStickX:
        report->rightStickX = m_lastValue;
        break;
    case GamepadRightStickY:
        report->rightStickY = m_lastValue;
        break;
    case GamepadLeftTrigger:
        report->leftTrigger = m_lastValue;
        break;
    case GamepadRightTrigger:
        report->rightTrigger = m_lastValue;
        break;
    default:
        break;
    }
}