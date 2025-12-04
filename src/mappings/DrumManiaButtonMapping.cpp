#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

DrumManiaButtonMapping::DrumManiaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
}

void DrumManiaButtonMapping::update_hid(uint8_t *buf)
{
}
void DrumManiaButtonMapping::update_wii(uint8_t *buf)
{
}
void DrumManiaButtonMapping::update_switch(uint8_t *buf)
{
}

void DrumManiaButtonMapping::update_ps2(uint8_t *buf)
{
    // TODO: Works like a GH ps2 guitar, but also holds dpad right
}

void DrumManiaButtonMapping::update_ps3(uint8_t *buf)
{
}

void DrumManiaButtonMapping::update_ps4(uint8_t *buf)
{
}

void DrumManiaButtonMapping::update_xinput(uint8_t *buf)
{
}
void DrumManiaButtonMapping::update_ogxbox(uint8_t *buf)
{
}