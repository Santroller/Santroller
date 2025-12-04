#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

KeyboardManiaAxisMapping::KeyboardManiaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : AxisMapping(mapping, std::move(input), id, true)
{
}

void KeyboardManiaAxisMapping::update_hid(uint8_t *buf)
{
}
void KeyboardManiaAxisMapping::update_wii(uint8_t *buf)
{
}
void KeyboardManiaAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing on switch
}

void KeyboardManiaAxisMapping::update_ps2(uint8_t *buf)
{
    // not a thing, ps2 controller was just hid based
}

void KeyboardManiaAxisMapping::update_ps3(uint8_t *buf)
{
}

void KeyboardManiaAxisMapping::update_ps4(uint8_t *buf)
{
    // not a thing on ps4
}

void KeyboardManiaAxisMapping::update_xinput(uint8_t *buf)
{
}
void KeyboardManiaAxisMapping::update_ogxbox(uint8_t *buf)
{
}