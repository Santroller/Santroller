#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

DrumManiaButtonMapping::DrumManiaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
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
    
}

void DrumManiaButtonMapping::update_ps3(uint8_t *buf)
{
    // this does exist on PS3, theres a little bit of info about it in rpcs3 but not much
    // would have to obtain the game and test it
}

void DrumManiaButtonMapping::update_ps4(uint8_t *buf)
{
}

void DrumManiaButtonMapping::update_ps5(uint8_t *buf)
{
}

void DrumManiaButtonMapping::update_xinput(uint8_t *buf)
{
}
void DrumManiaButtonMapping::update_ogxbox(uint8_t *buf)
{
}