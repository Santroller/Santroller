#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

SDVXButtonMapping::SDVXButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void SDVXButtonMapping::update_hid(uint8_t *buf)
{
}
void SDVXButtonMapping::update_wii(uint8_t *buf)
{
}
void SDVXButtonMapping::update_switch(uint8_t *buf)
{
}

void SDVXButtonMapping::update_ps2(uint8_t *buf)
{
}

void SDVXButtonMapping::update_ps3(uint8_t *buf)
{
}

void SDVXButtonMapping::update_ps4(uint8_t *buf)
{
}

void SDVXButtonMapping::update_ps5(uint8_t *buf)
{
}

void SDVXButtonMapping::update_xinput(uint8_t *buf)
{
}
void SDVXButtonMapping::update_ogxbox(uint8_t *buf)
{
}
void SDVXButtonMapping::update_xboxone(uint8_t *buf)
{
}