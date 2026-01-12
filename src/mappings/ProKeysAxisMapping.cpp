#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

ProKeysAxisMapping::ProKeysAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : AxisMapping(mapping, std::move(input), id, false)
{
}

void ProKeysAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void ProKeysAxisMapping::update_wii(uint8_t *buf)
{
}
void ProKeysAxisMapping::update_switch(uint8_t *buf)
{
}

void ProKeysAxisMapping::update_ps2(uint8_t *buf)
{
}

void ProKeysAxisMapping::update_ps3(uint8_t *buf)
{
}

void ProKeysAxisMapping::update_ps4(uint8_t *buf)
{
}

void ProKeysAxisMapping::update_ps5(uint8_t *buf)
{
}

void ProKeysAxisMapping::update_xinput(uint8_t *buf)
{
}
void ProKeysAxisMapping::update_ogxbox(uint8_t *buf)
{
}