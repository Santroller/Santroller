#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
// TODO: this
DJMaxTurntableButtonMapping::DJMaxTurntableButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void DJMaxTurntableButtonMapping::update_hid(uint8_t *buf)
{
}
void DJMaxTurntableButtonMapping::update_wii(uint8_t *buf)
{
}
void DJMaxTurntableButtonMapping::update_switch(uint8_t *buf)
{
}

void DJMaxTurntableButtonMapping::update_ps2(uint8_t *buf)
{
}

void DJMaxTurntableButtonMapping::update_ps3(uint8_t *buf)
{
}

void DJMaxTurntableButtonMapping::update_ps4(uint8_t *buf)
{
}

void DJMaxTurntableButtonMapping::update_ps5(uint8_t *buf)
{
}

void DJMaxTurntableButtonMapping::update_xinput(uint8_t *buf)
{
}
void DJMaxTurntableButtonMapping::update_ogxbox(uint8_t *buf)
{
}