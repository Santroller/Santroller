#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

BeatManiaButtonMapping::BeatManiaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
}

void BeatManiaButtonMapping::update_hid(uint8_t *buf)
{
    // not a thing
   
}
void BeatManiaButtonMapping::update_wii(uint8_t *buf)
{
    // not a thing
    
}
void BeatManiaButtonMapping::update_switch(uint8_t *buf)
{
    // not a thing
    
}

void BeatManiaButtonMapping::update_ps2(uint8_t *buf)
{
    // TODO: https://github.com/PCSX2/pcsx2/issues/10176
}

void BeatManiaButtonMapping::update_ps3(uint8_t *buf)
{
    // not a thing
    
}

void BeatManiaButtonMapping::update_ps4(uint8_t *buf)
{
    // not a thing
    
}

void BeatManiaButtonMapping::update_xinput(uint8_t *buf)
{
    // not a thing
    
}
void BeatManiaButtonMapping::update_ogxbox(uint8_t *buf)
{
    // not a thing
    
}