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
    // beatmaniaIIDX Controller 	PlayStation Digital Controller
    // Scratch Clockwise 	D-Pad Up
    // Scratch Counterclockwise 	D-Pad Down
    // Button 1 (F, White 1) 	Square
    // Button 2 (F#, Black 1) 	L1
    // Button 3 (G, White 2) 	Cross
    // Button 4 (G#, Black 2) 	R1
    // Button 5 (A, White 3) 	Circle
    // Button 6 (A#, Black 3) 	L2
    // Button 7 (B, White 4) 	D-Pad Left
    // Foot Pedal 	R2
    // Select 	Select
    // Start 	Start
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