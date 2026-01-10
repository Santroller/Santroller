#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
// TODO: this
// TODO: need to do some special handling of strum here, since there is one strum button
GuitarFreaksButtonMapping::GuitarFreaksButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
    
}

void GuitarFreaksButtonMapping::update_hid(uint8_t *buf)
{
    // PCGuitarFreaks_Data_t *report = (PCGuitarFreaks_Data_t *)buf;
    // switch (m_mapping.mapping.gfButton)
    // {
    // case GuitarFreaksGreen:
    //     report->a |= m_lastValue;
    //     break;
    // case GuitarFreaksRed:
    //     report->b |= m_lastValue;
    //     break;
    // case GuitarFreaksBlue:
    //     report->y |= m_lastValue;
    //     break;
    // case GuitarFreaksBack:
    //     report->back |= m_lastValue;
    //     break;
    // case GuitarFreaksStart:
    //     report->start |= m_lastValue;
    //     break;
    // case GuitarFreaksGuide:
    //     report->guide |= m_lastValue;
    //     break;
    // case GuitarFreaksStrum:
    //     report->dpadUp |= m_lastValue;
    //     break;
    // }
}
void GuitarFreaksButtonMapping::update_wii(uint8_t *buf)
{
    // no mapping for wii
}
void GuitarFreaksButtonMapping::update_switch(uint8_t *buf)
{
    // todo
}

void GuitarFreaksButtonMapping::update_ps2(uint8_t *buf)
{
    // TODO: this is a thing
}

void GuitarFreaksButtonMapping::update_ps3(uint8_t *buf)
{
    // TODO: this is also a thing
}

void GuitarFreaksButtonMapping::update_ps4(uint8_t *buf)
{
}

void GuitarFreaksButtonMapping::update_ps5(uint8_t *buf)
{
}

void GuitarFreaksButtonMapping::update_xinput(uint8_t *buf)
{
}
void GuitarFreaksButtonMapping::update_ogxbox(uint8_t *buf)
{
}