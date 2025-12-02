#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

PopNMusicButtonMapping::PopNMusicButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
    
}

void PopNMusicButtonMapping::update_hid(uint8_t *buf)
{
    // TODO: how does this work for hid
}
void PopNMusicButtonMapping::update_wii(uint8_t *buf)
{
    // TODO: how does this work on wii
}
void PopNMusicButtonMapping::update_switch(uint8_t *buf)
{
    
}

void PopNMusicButtonMapping::update_ps2(uint8_t *buf)
{
    // TODO: how does this work on ps2
}

void PopNMusicButtonMapping::update_ps3(uint8_t *buf)
{
    // not a thing
}

void PopNMusicButtonMapping::update_ps4(uint8_t *buf)
{
    // not a thing
}

void PopNMusicButtonMapping::update_xinput(uint8_t *buf)
{
   // not a thing
}
void PopNMusicButtonMapping::update_ogxbox(uint8_t *buf)
{
    // not a thing
}