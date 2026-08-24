#include "mappings/mapping.hpp"
#include "instance.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

SDVXAxisMapping::SDVXAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void SDVXAxisMapping::update_hid(uint8_t *buf)
{
}
void SDVXAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
}
void SDVXAxisMapping::update_switch(uint8_t *buf)
{
}

void SDVXAxisMapping::update_ps2(uint8_t *buf)
{
}

void SDVXAxisMapping::update_ps3(uint8_t *buf)
{
}

void SDVXAxisMapping::update_ps4(uint8_t *buf)
{
}

void SDVXAxisMapping::update_ps5(uint8_t *buf)
{
}

void SDVXAxisMapping::update_xinput(uint8_t *buf)
{
}
void SDVXAxisMapping::update_ogxbox(uint8_t *buf)
{
}
void SDVXAxisMapping::update_xboxone(uint8_t *buf)
{
}