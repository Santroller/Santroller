#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>
// TODO: this
GuitarFreaksAxisMapping::GuitarFreaksAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : AxisMapping(mapping, std::move(input), id, false)
{
}

void GuitarFreaksAxisMapping::update_hid(uint8_t *buf)
{
    // PCGuitarFreaks_Data_t *report = (PCGuitarFreaks_Data_t *)buf;
    // switch (m_mapping.mapping.gfAxis)
    // {
    // case GuitarFreaksLeftStickX:
    //     report->leftStickX = m_calibratedValue - 32767;
    // case GuitarFreaksLeftStickY:
    //     report->leftStickY = m_calibratedValue - 32767;
    //     break;
    // }
}
void GuitarFreaksAxisMapping::update_wii(uint8_t *buf)
{
}
void GuitarFreaksAxisMapping::update_switch(uint8_t *buf)
{
}

void GuitarFreaksAxisMapping::update_ps2(uint8_t *buf)
{
}

void GuitarFreaksAxisMapping::update_ps3(uint8_t *buf)
{
}

void GuitarFreaksAxisMapping::update_ps4(uint8_t *buf)
{
}

void GuitarFreaksAxisMapping::update_xinput(uint8_t *buf)
{
}
void GuitarFreaksAxisMapping::update_ogxbox(uint8_t *buf)
{
}