#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

PopNMusicAxisMapping::PopNMusicAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}

void PopNMusicAxisMapping::update_hid(uint8_t *buf)
{
    // TODO: how does this work for hid
    // https://github.com/whowechina/popn_pico

    // https://github.com/CrazyRedMachine/UltimatePopnController
}
void PopNMusicAxisMapping::update_wii(uint8_t *buf)
{
    // would need to emulate a remote here as it was motion based
}
void PopNMusicAxisMapping::update_switch(uint8_t *buf)
{
}

void PopNMusicAxisMapping::update_ps2(uint8_t *buf)
{
    // https://psx-spx.consoledev.net/controllersandmemorycards/#controllers-popn-controllers
    // holds left right and down
}

void PopNMusicAxisMapping::update_ps3(uint8_t *buf)
{
    // not a thing
}

void PopNMusicAxisMapping::update_ps4(uint8_t *buf)
{
    // not a thing
}

void PopNMusicAxisMapping::update_ps5(uint8_t *buf)
{
    // not a thing
}

void PopNMusicAxisMapping::update_xinput(uint8_t *buf)
{
    // not a thing
}
void PopNMusicAxisMapping::update_ogxbox(uint8_t *buf)
{
    // not a thing
}
void PopNMusicAxisMapping::update_xboxone(uint8_t *buf)
{
}