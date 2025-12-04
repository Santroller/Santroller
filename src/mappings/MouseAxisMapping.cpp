#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

MouseAxisMapping::MouseAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : AxisMapping(mapping, std::move(input), id, false)
{
}

void MouseAxisMapping::update_hid(uint8_t *buf)
{
    hid_mouse_report_t *report = (hid_mouse_report_t *)buf;
    switch (m_mapping.mapping.mouseAxis)
    {
    case MouseMoveX:
        report->x = (m_calibratedValue - 32767) >> 8;
        break;
    case MouseMoveY:
        report->y = (m_calibratedValue - 32767) >> 8;
        break;
    case MouseScrollX:
        report->wheel = (m_calibratedValue - 32767) >> 8;
        break;
    case MouseScrollY:
        report->pan = (m_calibratedValue - 32767) >> 8;
        break;
    }
}
void MouseAxisMapping::update_wii(uint8_t *buf)
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

void MouseAxisMapping::update_xinput(uint8_t *buf)
{
    // not a thing
}
void MouseAxisMapping::update_ogxbox(uint8_t *buf)
{
    // not a thing
}