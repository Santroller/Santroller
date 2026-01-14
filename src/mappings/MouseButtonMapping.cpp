#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

MouseButtonMapping::MouseButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void MouseButtonMapping::update_hid(uint8_t *buf)
{
    hid_mouse_report_t *report = (hid_mouse_report_t *)buf;
    switch (m_mapping.mapping.mouseButton)
    {
    case MouseLeft:
        report->buttons |= MOUSE_BUTTON_LEFT;
        break;
    case MouseMiddle:
        report->buttons |= MOUSE_BUTTON_MIDDLE;
        break;
    case MouseRight:
        report->buttons |= MOUSE_BUTTON_RIGHT;
        break;
    }
}
void MouseButtonMapping::update_wii(uint8_t *buf)
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