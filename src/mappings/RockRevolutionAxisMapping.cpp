#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

DrumManiaAxisMapping::DrumManiaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void DrumManiaAxisMapping::update_hid(uint8_t *buf)
{
}
void DrumManiaAxisMapping::update_wii(uint8_t *buf)
{
}
void DrumManiaAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing on switch
}

void DrumManiaAxisMapping::update_ps2(uint8_t *buf)
{
    // holds left and right
}

void DrumManiaAxisMapping::update_ps3(uint8_t *buf)
{
}

void DrumManiaAxisMapping::update_ps4(uint8_t *buf)
{
    // not a thing on ps4
}

void DrumManiaAxisMapping::update_ps5(uint8_t *buf)
{
    // not a thing on ps5
}

void DrumManiaAxisMapping::update_xinput(uint8_t *buf)
{
}
void DrumManiaAxisMapping::update_ogxbox(uint8_t *buf)
{
}
void DrumManiaAxisMapping::update_xboxone(uint8_t *buf)
{
}