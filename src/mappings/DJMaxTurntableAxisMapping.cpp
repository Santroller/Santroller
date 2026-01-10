#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>
// TODO: this
DJMaxTurntableAxisMapping::DJMaxTurntableAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : AxisMapping(mapping, std::move(input), id, false)
{
}

void DJMaxTurntableAxisMapping::update_hid(uint8_t *buf)
{
}
void DJMaxTurntableAxisMapping::update_wii(uint8_t *buf)
{
}
void DJMaxTurntableAxisMapping::update_switch(uint8_t *buf)
{
}

void DJMaxTurntableAxisMapping::update_ps2(uint8_t *buf)
{
}

void DJMaxTurntableAxisMapping::update_ps3(uint8_t *buf)
{
}

void DJMaxTurntableAxisMapping::update_ps4(uint8_t *buf)
{
}

void DJMaxTurntableAxisMapping::update_ps5(uint8_t *buf)
{
}

void DJMaxTurntableAxisMapping::update_xinput(uint8_t *buf)
{
}
void DJMaxTurntableAxisMapping::update_ogxbox(uint8_t *buf)
{
}