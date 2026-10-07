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

// L2 and R2 are triggers on gamepads, so buttons on them become trigger axes driven fully by the button
static proto_Mapping as_gamepad_trigger(proto_Mapping mapping, GamepadAxisType axis)
{
    mapping.mapping.which_mapping = proto_Output_gamepadAxis_tag;
    mapping.mapping.mapping.gamepadAxis = axis;
    mapping.has_pressed = true;
    mapping.pressed = UINT16_MAX;
    mapping.has_released = true;
    mapping.released = 0;
    mapping.has_center = true;
    mapping.center = 0;
    mapping.has_min = true;
    mapping.min = 0;
    mapping.has_max = true;
    mapping.max = UINT16_MAX;
    return mapping;
}

// pop'n buttons are just PS buttons on a digital pad, so they are handled as the equivalent
// gamepad output.
// https://github.com/PCSX2/pcsx2/blob/master/pcsx2/SIO/Pad/PadPopn.cpp
proto_Mapping popn_as_gamepad(proto_Mapping mapping)
{
    GamepadButtonType button = Gamepad_A;
    switch (mapping.mapping.mapping.popnButton)
    {
    case PopNMusic_Button1:
        button = Gamepad_Y;
        break;
    case PopNMusic_Button2:
        button = Gamepad_B;
        break;
    case PopNMusic_Button3:
        button = Gamepad_RightShoulder;
        break;
    case PopNMusic_Button4:
        button = Gamepad_A;
        break;
    case PopNMusic_Button5:
        button = Gamepad_LeftShoulder;
        break;
    case PopNMusic_Button6:
        button = Gamepad_X;
        break;
    case PopNMusic_Button7:
        return as_gamepad_trigger(mapping, Gamepad_RightTrigger);
    case PopNMusic_Button9:
        return as_gamepad_trigger(mapping, Gamepad_LeftTrigger);
    case PopNMusic_Button8:
        button = Gamepad_DpadUp;
        break;
    }
    mapping.mapping.which_mapping = proto_Output_gamepadButton_tag;
    mapping.mapping.mapping.gamepadButton = button;
    return mapping;
}

// beatmania IIDX controllers are also PS digital pads, so they are handled as the equivalent gamepad output.
// https://github.com/PCSX2/pcsx2/issues/10176
proto_Mapping beatmania_as_gamepad(proto_Mapping mapping)
{
    GamepadButtonType button = Gamepad_A;
    switch (mapping.mapping.mapping.bmButton)
    {
    // Button 1 (F, White 1): Square
    case BeatMania_Button1:
        button = Gamepad_X;
        break;
    // Button 2 (F#, Black 1): L1
    case BeatMania_Button2:
        button = Gamepad_LeftShoulder;
        break;
    // Button 3 (G, White 2): Cross
    case BeatMania_Button3:
        button = Gamepad_A;
        break;
    // Button 4 (G#, Black 2): R1
    case BeatMania_Button4:
        button = Gamepad_RightShoulder;
        break;
    // Button 5 (A, White 3): Circle
    case BeatMania_Button5:
        button = Gamepad_B;
        break;
    // Button 6 (A#, Black 3): L2
    case BeatMania_Button6:
        return as_gamepad_trigger(mapping, Gamepad_LeftTrigger);
    // Button 7 (B, White 4): D-Pad Left
    case BeatMania_Button7:
        button = Gamepad_DpadLeft;
        break;
    // Scratch Clockwise: D-Pad Up
    case BeatMania_ScratchClockwise:
        button = Gamepad_DpadUp;
        break;
    // Scratch Counterclockwise: D-Pad Down
    case BeatMania_ScratchCounterClockwise:
        button = Gamepad_DpadDown;
        break;
    // Foot Pedal: R2
    case BeatMania_Pedal:
        return as_gamepad_trigger(mapping, Gamepad_RightTrigger);
    }
    mapping.mapping.which_mapping = proto_Output_gamepadButton_tag;
    mapping.mapping.mapping.gamepadButton = button;
    return mapping;
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
