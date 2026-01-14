#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

SVDXAxisMapping::SVDXAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void SVDXAxisMapping::update_hid(uint8_t *buf)
{
}
void SVDXAxisMapping::update_wii(uint8_t *buf)
{
}
void SVDXAxisMapping::update_switch(uint8_t *buf)
{
}

void SVDXAxisMapping::update_ps2(uint8_t *buf)
{
}

void SVDXAxisMapping::update_ps3(uint8_t *buf)
{
}

void SVDXAxisMapping::update_ps4(uint8_t *buf)
{
}

void SVDXAxisMapping::update_ps5(uint8_t *buf)
{
}

void SVDXAxisMapping::update_xinput(uint8_t *buf)
{
}
void SVDXAxisMapping::update_ogxbox(uint8_t *buf)
{
}