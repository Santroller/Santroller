#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

TaikoButtonMapping::TaikoButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void TaikoButtonMapping::update_hid(uint8_t *buf)
{
    
}
void TaikoButtonMapping::update_wii(uint8_t *buf)
{
    
}
void TaikoButtonMapping::update_switch(uint8_t *buf)
{
    
}

void TaikoButtonMapping::update_ps2(uint8_t *buf)
{
    // https://github.com/PCSX2/pcsx2/issues/4763#issuecomment-2269081492
}

void TaikoButtonMapping::update_ps3(uint8_t *buf)
{
}
   

void TaikoButtonMapping::update_ps4(uint8_t *buf)
{
    
}
   

void TaikoButtonMapping::update_ps5(uint8_t *buf)
{
    
}

void TaikoButtonMapping::update_xinput(uint8_t *buf)
{
    
}
void TaikoButtonMapping::update_ogxbox(uint8_t *buf)
{
    
}