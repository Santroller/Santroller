#include "class/hid/hid.h"
#include "config/config.hpp"
#include "events.pb.h"
#include "instance.hpp"
#include "main.hpp"
#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include <pb_encode.h>
#include <stdint.h>
#include <utils.h>

WheelButtonMapping::WheelButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void WheelButtonMapping::update_hid(uint8_t *buf)
{
    
}
void WheelButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    
}
void WheelButtonMapping::update_switch(uint8_t *buf)
{
    
}

void WheelButtonMapping::update_ps2(uint8_t *buf)
{
    
}

void WheelButtonMapping::update_ps3(uint8_t *buf)
{
    
}

void WheelButtonMapping::update_ps4(uint8_t *buf)
{
    
}

void WheelButtonMapping::update_ps5(uint8_t *buf)
{
    
}

void WheelButtonMapping::update_xinput(uint8_t *buf)
{
    
}
void WheelButtonMapping::update_ogxbox(uint8_t *buf)
{
    
}
void WheelButtonMapping::update_xboxone(uint8_t *buf)
{
}

WheelAxisMapping::WheelAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}

void WheelAxisMapping::update_hid(uint8_t *buf)
{
    
}
void WheelAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    
}
void WheelAxisMapping::update_switch(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_ps2(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_ps3(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_ps4(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_ps5(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_xinput(uint8_t *buf)
{
    
}
void WheelAxisMapping::update_ogxbox(uint8_t *buf)
{
    
}
void WheelAxisMapping::update_xboxone(uint8_t *buf)
{
}

KeyboardButtonMapping::KeyboardButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void KeyboardButtonMapping::update_hid(uint8_t *buf)
{
    if (m_lastValue) {
        m_profile->keyboard_state.pressedKeys |= 1 << m_mapping.mapping.mapping.keycode;
    }
}
void KeyboardButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    
}
void KeyboardButtonMapping::update_switch(uint8_t *buf)
{
    
}

void KeyboardButtonMapping::update_ps2(uint8_t *buf)
{
    
}

void KeyboardButtonMapping::update_ps3(uint8_t *buf)
{
    
}

void KeyboardButtonMapping::update_ps4(uint8_t *buf)
{
    
}

void KeyboardButtonMapping::update_ps5(uint8_t *buf)
{
    
}

void KeyboardButtonMapping::update_xinput(uint8_t *buf)
{
    
}
void KeyboardButtonMapping::update_ogxbox(uint8_t *buf)
{
    
}
void KeyboardButtonMapping::update_xboxone(uint8_t *buf)
{
}

MouseAxisMapping::MouseAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}

void MouseAxisMapping::update_hid(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    hid_mouse_report_t *report = (hid_mouse_report_t *)buf;
    switch (m_mapping.mapping.mapping.mouseAxis)
    {
    case Mouse_MoveX:
        report->x = (m_calibratedValue - 32768) >> 8;
        break;
    case Mouse_MoveY:
        report->y = (m_calibratedValue - 32768) >> 8;
        break;
    case Mouse_ScrollX:
        report->wheel = (m_calibratedValue - 32768) >> 8;
        break;
    case Mouse_ScrollY:
        report->pan = (m_calibratedValue - 32768) >> 8;
        break;
    }
}
void MouseAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // TODO: not a thing currently but we could map to the wii cursor maybe
}
void MouseAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void MouseAxisMapping::update_ps2(uint8_t *buf)
{
    // TODO: this does exist
}

void MouseAxisMapping::update_ps3(uint8_t *buf)
{
    // not a thing
}

void MouseAxisMapping::update_ps4(uint8_t *buf)
{
    // not a thing
}

void MouseAxisMapping::update_ps5(uint8_t *buf)
{
    // not a thing
}

void MouseAxisMapping::update_xinput(uint8_t *buf)
{
    // not a thing
}
void MouseAxisMapping::update_ogxbox(uint8_t *buf)
{
    // not a thing
}
void MouseAxisMapping::update_xboxone(uint8_t *buf)
{
}

MouseButtonMapping::MouseButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void MouseButtonMapping::update_hid(uint8_t *buf)
{
    hid_mouse_report_t *report = (hid_mouse_report_t *)buf;
    switch (m_mapping.mapping.mapping.mouseButton)
    {
    case Mouse_Left:
        report->buttons |= MOUSE_BUTTON_LEFT;
        break;
    case Mouse_Middle:
        report->buttons |= MOUSE_BUTTON_MIDDLE;
        break;
    case Mouse_Right:
        report->buttons |= MOUSE_BUTTON_RIGHT;
        break;
    }
}
void MouseButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void MouseButtonMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void MouseButtonMapping::update_ps2(uint8_t *buf)
{
    // this one is a thing
}

void MouseButtonMapping::update_ps3(uint8_t *buf)
{
    // not a thing
}

void MouseButtonMapping::update_ps4(uint8_t *buf)
{
    // not a thing
}

void MouseButtonMapping::update_ps5(uint8_t *buf)
{
    // not a thing
}

void MouseButtonMapping::update_xinput(uint8_t *buf)
{
    // not a thing
}
void MouseButtonMapping::update_ogxbox(uint8_t *buf)
{
    // not a thing
}
void MouseButtonMapping::update_xboxone(uint8_t *buf)
{
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
    // TODO: wii + ps2 + xone need to be handled a bit differently, since the formats aren't the same as gamepad
    if (format == 1)
    {
        WiiClassicDataFormat1_t *report = (WiiClassicDataFormat1_t *)buf;
        switch (m_mapping.mapping.mapping.gamepadButton)
        {
        case Gamepad_A:
            report->a |= m_lastValue;
            break;
        case Gamepad_B:
            report->b |= m_lastValue;
            break;
        case Gamepad_X:
            report->x |= m_lastValue;
            break;
        case Gamepad_Y:
            report->y |= m_lastValue;
            break;
        case Gamepad_Start:
            report->start |= m_lastValue;
            break;
        case Gamepad_Back:
            report->back |= m_lastValue;
            break;
        case Gamepad_Capture:
            // report->capture |= m_lastValue;
            break;
        case Gamepad_Guide:
            report->guide |= m_lastValue;
            break;
        case Gamepad_LeftShoulder:
            report->leftShoulder |= m_lastValue;
            break;
        case Gamepad_RightShoulder:
            report->rightShoulder |= m_lastValue;
            break;
        case Gamepad_DpadUp:
            report->dpadUp |= m_lastValue;
            break;
        case Gamepad_DpadDown:
            report->dpadDown |= m_lastValue;
            break;
        case Gamepad_DpadLeft:
            report->dpadLeft |= m_lastValue;
            break;
        case Gamepad_DpadRight:
            report->dpadRight |= m_lastValue;
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
            report->a |= m_lastValue;
            break;
        case Gamepad_B:
            report->b |= m_lastValue;
            break;
        case Gamepad_X:
            report->x |= m_lastValue;
            break;
        case Gamepad_Y:
            report->y |= m_lastValue;
            break;
        case Gamepad_Start:
            report->start |= m_lastValue;
            break;
        case Gamepad_Back:
            report->back |= m_lastValue;
            break;
        case Gamepad_Capture:
            // report->capture |= m_lastValue;
            break;
        case Gamepad_Guide:
            report->guide |= m_lastValue;
            break;
        case Gamepad_LeftShoulder:
            report->leftShoulder |= m_lastValue;
            break;
        case Gamepad_RightShoulder:
            report->rightShoulder |= m_lastValue;
            break;
        case Gamepad_DpadUp:
            report->dpadUp |= m_lastValue;
            break;
        case Gamepad_DpadDown:
            report->dpadDown |= m_lastValue;
            break;
        case Gamepad_DpadLeft:
            report->dpadLeft |= m_lastValue;
            break;
        case Gamepad_DpadRight:
            report->dpadRight |= m_lastValue;
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
            report->a |= m_lastValue;
            break;
        case Gamepad_B:
            report->b |= m_lastValue;
            break;
        case Gamepad_X:
            report->x |= m_lastValue;
            break;
        case Gamepad_Y:
            report->y |= m_lastValue;
            break;
        case Gamepad_Start:
            report->start |= m_lastValue;
            break;
        case Gamepad_Back:
            report->back |= m_lastValue;
            break;
        case Gamepad_Capture:
            // report->capture |= m_lastValue;
            break;
        case Gamepad_Guide:
            report->guide |= m_lastValue;
            break;
        case Gamepad_LeftShoulder:
            report->leftShoulder |= m_lastValue;
            break;
        case Gamepad_RightShoulder:
            report->rightShoulder |= m_lastValue;
            break;
        case Gamepad_DpadUp:
            report->dpadUp |= m_lastValue;
            break;
        case Gamepad_DpadDown:
            report->dpadDown |= m_lastValue;
            break;
        case Gamepad_DpadLeft:
            report->dpadLeft |= m_lastValue;
            break;
        case Gamepad_DpadRight:
            report->dpadRight |= m_lastValue;
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
    case Gamepad_A: buttons->a |= m_lastValue; break;
    case Gamepad_B: buttons->b |= m_lastValue; break;
    case Gamepad_Back: buttons->minus |= m_lastValue; break;
    case Gamepad_Start: buttons->plus |= m_lastValue; break;
    case Gamepad_Guide: buttons->home |= m_lastValue; break;
    case Gamepad_DpadUp: buttons->up |= m_lastValue; break;
    case Gamepad_DpadDown: buttons->down |= m_lastValue; break;
    case Gamepad_DpadLeft: buttons->left |= m_lastValue; break;
    case Gamepad_DpadRight: buttons->right |= m_lastValue; break;
    default: break;
    }
}

void GamepadButtonMapping::update_switch(uint8_t *buf)
{
    SwitchInputReport *report = (SwitchInputReport *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        report->capture |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
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
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Capture:
        // report->capture |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
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
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        report->capture |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
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
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        report->capture |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
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
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        report->touchpad |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
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
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        report->capture |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
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
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
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
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        if (m_lastValue)
        {
            report->consoleFunctions[0] = 0x01;
        }
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
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
        report->a |= m_lastValue;
        break;
    case Gamepad_B:
        report->b |= m_lastValue;
        break;
    case Gamepad_X:
        report->x |= m_lastValue;
        break;
    case Gamepad_Y:
        report->y |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Guide:
        report->guide |= m_lastValue;
        break;
    case Gamepad_Capture:
        report->capture |= m_lastValue;
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
    return;
}
