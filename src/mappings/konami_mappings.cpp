#include "events.pb.h"
#include "instance.hpp"
#include "main.hpp"
#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include <pb_encode.h>
#include <stdint.h>
#include <utils.h>

// TODO: this
// TODO: need to do some special handling of strum here, since there is one strum button
GuitarFreaksButtonMapping::GuitarFreaksButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void GuitarFreaksButtonMapping::update_hid(uint8_t *buf)
{
    // PCGuitarFreaks_Data_t *report = (PCGuitarFreaks_Data_t *)buf;
    // switch (m_mapping.mapping.mapping.gfButton)
    // {
    // case GuitarFreaksGreen:
    //     report->a |= m_last_value;
    //     break;
    // case GuitarFreaksRed:
    //     report->b |= m_last_value;
    //     break;
    // case GuitarFreaksBlue:
    //     report->y |= m_last_value;
    //     break;
    // case GuitarFreaksBack:
    //     report->back |= m_last_value;
    //     break;
    // case GuitarFreaksStart:
    //     report->start |= m_last_value;
    //     break;
    // case GuitarFreaksGuide:
    //     report->guide |= m_last_value;
    //     break;
    // case GuitarFreaksStrum:
    //     report->dpadUp |= m_last_value;
    //     break;
    // }
}
void GuitarFreaksButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // no mapping for wii
}
void GuitarFreaksButtonMapping::update_switch(uint8_t *buf)
{
    // todo
}

void GuitarFreaksButtonMapping::update_ps2(uint8_t *buf)
{
    // TODO: this is a thing
}

void GuitarFreaksButtonMapping::update_ps3(uint8_t *buf)
{
    // in the ps3 case, we would actually need to emulate a ds3 that has right+left held at all times,
    // since that would then let us use this with pademu and should work with ps2 on ps3 too.
}

void GuitarFreaksButtonMapping::update_ps4(uint8_t *buf)
{
}

void GuitarFreaksButtonMapping::update_ps5(uint8_t *buf)
{
}

void GuitarFreaksButtonMapping::update_xinput(uint8_t *buf)
{
}
void GuitarFreaksButtonMapping::update_ogxbox(uint8_t *buf)
{
}
void GuitarFreaksButtonMapping::update_xboxone(uint8_t *buf)
{
}

DrumManiaButtonMapping::DrumManiaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void DrumManiaButtonMapping::update_hid(uint8_t *buf)
{
}
void DrumManiaButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
}
void DrumManiaButtonMapping::update_switch(uint8_t *buf)
{
}

void DrumManiaButtonMapping::update_ps2(uint8_t *buf)
{
    // TODO: Works like a GH ps2 guitar, but also holds dpad right
}

void DrumManiaButtonMapping::update_ps3(uint8_t *buf)
{
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
void DrumManiaButtonMapping::update_xboxone(uint8_t *buf)
{
    // not a thing
}

DrumManiaAxisMapping::DrumManiaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void DrumManiaAxisMapping::update_hid(uint8_t *buf)
{
}
void DrumManiaAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
}
void DrumManiaAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing on switch
}

void DrumManiaAxisMapping::update_ps2(uint8_t *buf)
{
    // holds left and right
}

void DrumManiaAxisMapping::update_ps3(uint8_t *buf)
{
}

void DrumManiaAxisMapping::update_ps4(uint8_t *buf)
{
    // not a thing on ps4
}

void DrumManiaAxisMapping::update_ps5(uint8_t *buf)
{
    // not a thing on ps5
}

void DrumManiaAxisMapping::update_xinput(uint8_t *buf)
{
}
void DrumManiaAxisMapping::update_ogxbox(uint8_t *buf)
{
}
void DrumManiaAxisMapping::update_xboxone(uint8_t *buf)
{
    // not a thing
}

PopNMusicAxisMapping::PopNMusicAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}

void PopNMusicAxisMapping::update_hid(uint8_t *buf)
{
    // TODO: how does this work for hid
    // https://github.com/whowechina/popn_pico

    // https://github.com/CrazyRedMachine/UltimatePopnController
}
void PopNMusicAxisMapping::update_wii(uint8_t format, uint8_t *buf)
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

PopNMusicButtonMapping::PopNMusicButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void PopNMusicButtonMapping::update_hid(uint8_t *buf)
{
    // TODO: how does this work for hid
}
void PopNMusicButtonMapping::update_wii(uint8_t format, uint8_t *buf)
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

void PopNMusicButtonMapping::update_ps5(uint8_t *buf)
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
void PopNMusicButtonMapping::update_xboxone(uint8_t *buf)
{
}

BeatManiaButtonMapping::BeatManiaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void BeatManiaButtonMapping::update_hid(uint8_t *buf)
{
    // not a thing
   
}
void BeatManiaButtonMapping::update_wii(uint8_t format, uint8_t *buf)
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

void BeatManiaButtonMapping::update_ps5(uint8_t *buf)
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
void BeatManiaButtonMapping::update_xboxone(uint8_t *buf)
{
    // not a thing
}

BeatManiaAxisMapping::BeatManiaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void BeatManiaAxisMapping::update_hid(uint8_t *buf)
{
    // not a thing
}
void BeatManiaAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void BeatManiaAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void BeatManiaAxisMapping::update_ps2(uint8_t *buf)
{
    // TODO: https://github.com/PCSX2/pcsx2/issues/10176
}

void BeatManiaAxisMapping::update_ps3(uint8_t *buf)
{
    // not a thing
}

void BeatManiaAxisMapping::update_ps4(uint8_t *buf)
{
    // not a thing
}

void BeatManiaAxisMapping::update_ps5(uint8_t *buf)
{
    // not a thing
}

void BeatManiaAxisMapping::update_xinput(uint8_t *buf)
{
    // not a thing
}
void BeatManiaAxisMapping::update_ogxbox(uint8_t *buf)
{
    // not a thing
}
void BeatManiaAxisMapping::update_xboxone(uint8_t *buf)
{
    // not a thing
}

KeyboardManiaButtonMapping::KeyboardManiaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void KeyboardManiaButtonMapping::update_hid(uint8_t *buf)
{
}
void KeyboardManiaButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
}
void KeyboardManiaButtonMapping::update_switch(uint8_t *buf)
{
}

void KeyboardManiaButtonMapping::update_ps2(uint8_t *buf)
{
    // not a thing, ps2 controller was just hid based
}

void KeyboardManiaButtonMapping::update_ps3(uint8_t *buf)
{
}

void KeyboardManiaButtonMapping::update_ps4(uint8_t *buf)
{
}

void KeyboardManiaButtonMapping::update_ps5(uint8_t *buf)
{
}

void KeyboardManiaButtonMapping::update_xinput(uint8_t *buf)
{
}
void KeyboardManiaButtonMapping::update_ogxbox(uint8_t *buf)
{
}
void KeyboardManiaButtonMapping::update_xboxone(uint8_t *buf)
{
}

KeyboardManiaAxisMapping::KeyboardManiaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void KeyboardManiaAxisMapping::update_hid(uint8_t *buf)
{
}
void KeyboardManiaAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
}
void KeyboardManiaAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing on switch
}

void KeyboardManiaAxisMapping::update_ps2(uint8_t *buf)
{
    // not a thing, ps2 controller was just hid based
}

void KeyboardManiaAxisMapping::update_ps3(uint8_t *buf)
{
}

void KeyboardManiaAxisMapping::update_ps4(uint8_t *buf)
{
    // not a thing on ps4
}

void KeyboardManiaAxisMapping::update_ps5(uint8_t *buf)
{
    // not a thing on ps5
}

void KeyboardManiaAxisMapping::update_xinput(uint8_t *buf)
{
}
void KeyboardManiaAxisMapping::update_ogxbox(uint8_t *buf)
{
}
void KeyboardManiaAxisMapping::update_xboxone(uint8_t *buf)
{
}

SDVXButtonMapping::SDVXButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void SDVXButtonMapping::update_hid(uint8_t *buf)
{
}
void SDVXButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
}
void SDVXButtonMapping::update_switch(uint8_t *buf)
{
}

void SDVXButtonMapping::update_ps2(uint8_t *buf)
{
}

void SDVXButtonMapping::update_ps3(uint8_t *buf)
{
}

void SDVXButtonMapping::update_ps4(uint8_t *buf)
{
}

void SDVXButtonMapping::update_ps5(uint8_t *buf)
{
}

void SDVXButtonMapping::update_xinput(uint8_t *buf)
{
}
void SDVXButtonMapping::update_ogxbox(uint8_t *buf)
{
}
void SDVXButtonMapping::update_xboxone(uint8_t *buf)
{
}

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
