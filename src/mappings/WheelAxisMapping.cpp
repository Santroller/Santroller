#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

WheelAxisMapping::WheelAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}

void WheelAxisMapping::update_hid(uint8_t *buf)
{
    
}
void WheelAxisMapping::update_wii(uint8_t *buf)
{
    
}
void WheelAxisMapping::update_switch(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_ps2(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_ps3(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_ps4(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_ps5(uint8_t *buf)
{
    
}

void WheelAxisMapping::update_xinput(uint8_t *buf)
{
    
}
void WheelAxisMapping::update_ogxbox(uint8_t *buf)
{
    
}