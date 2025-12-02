#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

ProKeysButtonMapping::ProKeysButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : ButtonMapping(mapping, std::move(input), id)
{
    
}

void ProKeysButtonMapping::update_hid(uint8_t *buf)
{
    
}
void ProKeysButtonMapping::update_wii(uint8_t *buf)
{
    
}
void ProKeysButtonMapping::update_switch(uint8_t *buf)
{
    
}

void ProKeysButtonMapping::update_ps2(uint8_t *buf)
{
    
}

void ProKeysButtonMapping::update_ps3(uint8_t *buf)
{
    
}

void ProKeysButtonMapping::update_ps4(uint8_t *buf)
{
    
}

void ProKeysButtonMapping::update_xinput(uint8_t *buf)
{
    
}
void ProKeysButtonMapping::update_ogxbox(uint8_t *buf)
{
    
}