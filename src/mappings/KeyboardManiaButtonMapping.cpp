#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

KeyboardManiaButtonMapping::KeyboardManiaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
}

void KeyboardManiaButtonMapping::update_hid(uint8_t *buf)
{
}
void KeyboardManiaButtonMapping::update_wii(uint8_t *buf)
{
}
void KeyboardManiaButtonMapping::update_switch(uint8_t *buf)
{
}

void KeyboardManiaButtonMapping::update_ps2(uint8_t *buf)
{
    // not a thing, ps2 controller was just hid based
}

void KeyboardManiaButtonMapping::update_ps3(uint8_t *buf)
{
}

void KeyboardManiaButtonMapping::update_ps4(uint8_t *buf)
{
}

void KeyboardManiaButtonMapping::update_ps5(uint8_t *buf)
{
}

void KeyboardManiaButtonMapping::update_xinput(uint8_t *buf)
{
}
void KeyboardManiaButtonMapping::update_ogxbox(uint8_t *buf)
{
}