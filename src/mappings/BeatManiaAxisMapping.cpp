#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

BeatManiaAxisMapping::BeatManiaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void BeatManiaAxisMapping::update_hid(uint8_t *buf)
{
    // not a thing
}
void BeatManiaAxisMapping::update_wii(uint8_t *buf)
{
    // not a thing
}
void BeatManiaAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void BeatManiaAxisMapping::update_ps2(uint8_t *buf)
{
    // TODO: https://github.com/PCSX2/pcsx2/issues/10176
}

void BeatManiaAxisMapping::update_ps3(uint8_t *buf)
{
    // not a thing
}

void BeatManiaAxisMapping::update_ps4(uint8_t *buf)
{
    // not a thing
}

void BeatManiaAxisMapping::update_ps5(uint8_t *buf)
{
    // not a thing
}

void BeatManiaAxisMapping::update_xinput(uint8_t *buf)
{
    // not a thing
}
void BeatManiaAxisMapping::update_ogxbox(uint8_t *buf)
{
    // not a thing
}
void BeatManiaAxisMapping::update_xboxone(uint8_t *buf)
{
    // not a thing
}