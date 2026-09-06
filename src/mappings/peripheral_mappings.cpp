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
    if (m_last_value) {
        m_profile->keyboard_state.pressed_keys |= 1 << m_mapping.mapping.mapping.keycode;
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
        report->x = (m_calibrated_value - 32768) >> 8;
        break;
    case Mouse_MoveY:
        report->y = (m_calibrated_value - 32768) >> 8;
        break;
    case Mouse_ScrollX:
        report->wheel = (m_calibrated_value - 32768) >> 8;
        break;
    case Mouse_ScrollY:
        report->pan = (m_calibrated_value - 32768) >> 8;
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
