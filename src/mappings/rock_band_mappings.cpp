#include "events.pb.h"
#include "instance.hpp"
#include "main.hpp"
#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include <pb_encode.h>
#include <stdint.h>
#include <utils.h>

RockBandGuitarButtonMapping::RockBandGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void RockBandGuitarButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void RockBandGuitarButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
   // not a thing, was hid
}
void RockBandGuitarButtonMapping::update_switch(uint8_t *buf)
{
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_last_value;
        break;
    case RockBandGuitar_Red:
        report->b |= m_last_value;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_last_value;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_last_value;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_last_value;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_last_value;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_last_value;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_last_value;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_last_value;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_last_value;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_last_value;
        break;
    }
}

void RockBandGuitarButtonMapping::update_ps2(uint8_t *buf)
{
    // was hid
}

void RockBandGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3RockBandGuitar_Data_t *report = (PS3RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_last_value;
        break;
    case RockBandGuitar_Red:
        report->b |= m_last_value;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_last_value;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_last_value;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_last_value;
        break;
    case RockBandGuitar_Pedal:
        report->tilt |= m_last_value;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_last_value;
        report->solo |= m_last_value;
        break;
    }
}

void RockBandGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_last_value;
        break;
    case RockBandGuitar_Red:
        report->b |= m_last_value;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_last_value;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_last_value;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_last_value;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_last_value;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_last_value;
        report->solo |= m_last_value;
        break;
    }
}

void RockBandGuitarButtonMapping::update_ps5(uint8_t *buf)
{
    PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_last_value;
        break;
    case RockBandGuitar_Red:
        report->b |= m_last_value;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_last_value;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_last_value;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_last_value;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_last_value;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_last_value;
        report->solo |= m_last_value;
        break;
    }
}

void RockBandGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandGuitar_Data_t *report = (XInputRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_last_value;
        break;
    case RockBandGuitar_Red:
        report->b |= m_last_value;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_last_value;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_last_value;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_last_value;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_last_value;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_last_value;
        report->solo |= m_last_value;
        break;
    }
}
void RockBandGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandGuitar_Data_t *report = (OGXboxRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_last_value;
        break;
    case RockBandGuitar_Red:
        report->b |= m_last_value;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_last_value;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_last_value;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_last_value;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_last_value;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_last_value;
        report->solo |= m_last_value;
        break;
    }
}
void RockBandGuitarButtonMapping::update_xboxone(uint8_t *buf)
{
    XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_last_value;
        break;
    case RockBandGuitar_Red:
        report->b |= m_last_value;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_last_value;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_last_value;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_last_value;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_last_value;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_last_value;
        report->solo |= m_last_value;
        break;
    }
}

static uint8_t pickupUniversal[] = {0x19, 0x4c, 0x96, 0xb2, 0xe5};
static uint8_t pickupXb1[] = {0x00, 0x10, 0x20, 0x30, 0x40};
RockBandGuitarAxisMapping::RockBandGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.rbAxis == RockBandGuitar_Whammy)
{
}

void RockBandGuitarAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void RockBandGuitarAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    // TODO: we have to deal with data formats probably
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_Whammy:
        report->whammy = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Tilt:
        // report->tilt = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}
void RockBandGuitarAxisMapping::update_switch(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_Whammy:
        report->whammy = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}

void RockBandGuitarAxisMapping::update_ps2(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_Whammy:
        report->whammy = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}

void RockBandGuitarAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS3RockBandGuitar_Data_t *report = (PS3RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_Whammy:
        report->whammy = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = pickupUniversal[m_calibrated_value];
        break;
    }
}

void RockBandGuitarAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_Whammy:
        report->whammy = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = m_calibrated_value;
        break;
    default:
        break;
    }
}

void RockBandGuitarAxisMapping::update_ps5(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_Whammy:
        report->whammy = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = m_calibrated_value;
        break;
    default:
        break;
    }
}

void RockBandGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputRockBandGuitar_Data_t *report = (XInputRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_Whammy:
        report->whammy = m_calibrated_value - 32768;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibrated_value - 32768;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = pickupUniversal[m_calibrated_value];
        break;
    default:
        break;
    }
}
void RockBandGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxRockBandGuitar_Data_t *report = (OGXboxRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_Whammy:
        report->whammy = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = pickupUniversal[m_calibrated_value];
        break;
    default:
        break;
    }
}
void RockBandGuitarAxisMapping::update_xboxone(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_Whammy:
        report->whammy = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = pickupXb1[m_calibrated_value];
        break;
    default:
        break;
    }
}
RockBandGuitarGamepadAxisMapping::~RockBandGuitarGamepadAxisMapping() {}

RockBandGuitarGamepadAxisMapping::RockBandGuitarGamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadAxisMapping(mapping, std::move(input), id, profile)
{
}
void RockBandGuitarGamepadAxisMapping::update_xboxone(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        report->joystickX = m_calibrated_value - 32768;
        break;
    case Gamepad_LeftStickY:
        report->joystickY = m_calibrated_value - 32768;
        break;
    case Gamepad_RightStickX:
        // report->rightStickX = m_calibrated_value - 32768;
        break;
    case Gamepad_RightStickY:
        // report->rightStickY = m_calibrated_value - 32768;
        break;
    case Gamepad_LeftTrigger:
        // report->leftTrigger = m_calibrated_value >> 6;
        break;
    case Gamepad_RightTrigger:
        // report->rightTrigger = m_calibrated_value >> 6;
        break;
    default:
        break;
    }
}

RockBandDrumsButtonMapping::RockBandDrumsButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void RockBandDrumsButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void RockBandDrumsButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void RockBandDrumsButtonMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void RockBandDrumsButtonMapping::update_ps2(uint8_t *buf)
{
    // Not a thing - drums were always usb here
}

void RockBandDrumsButtonMapping::update_ps3(uint8_t *buf)
{
    PS3RockBandDrums_Data_t *report = (PS3RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_Kick1Pedal:
        report->kick1 |= m_last_value;
        break;
    case RockBandDrums_Kick2Pedal:
        report->kick2 |= m_last_value;
        break;
    default:
        break;
    }
}

void RockBandDrumsButtonMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandDrums_Data_t *report = (PS4RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_Kick1Pedal:
        report->kick1 |= m_last_value;
        break;
    case RockBandDrums_Kick2Pedal:
        report->kick2 |= m_last_value;
        break;
    default:
        break;
    }
}

void RockBandDrumsButtonMapping::update_ps5(uint8_t *buf)
{
    PS5RockBandDrums_Data_t *report = (PS5RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_Kick1Pedal:
        report->kick1 |= m_last_value;
        break;
    case RockBandDrums_Kick2Pedal:
        report->kick2 |= m_last_value;
        break;
    default:
        break;
    }
}

void RockBandDrumsButtonMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_Kick1Pedal:
        report->kick1 |= m_last_value;
        break;
    case RockBandDrums_Kick2Pedal:
        report->kick2 |= m_last_value;
        break;
    }
}
void RockBandDrumsButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandDrums_Data_t *report = (OGXboxRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_Kick1Pedal:
        report->kick1 |= m_last_value;
        break;
    case RockBandDrums_Kick2Pedal:
        report->kick2 |= m_last_value;
        break;
    default:
        break;
    }
}
void RockBandDrumsButtonMapping::update_xboxone(uint8_t *buf)
{
    
    XboxOneRockBandDrums_Data_t *report = (XboxOneRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_Kick1Pedal:
        report->leftShoulder |= m_last_value;
        break;
    case RockBandDrums_Kick2Pedal:
        report->rightShoulder |= m_last_value;
        break;
    }
}

RockBandDrumsAxisMapping::RockBandDrumsAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void RockBandDrumsAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void RockBandDrumsAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void RockBandDrumsAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing on switch
}

void RockBandDrumsAxisMapping::update_ps2(uint8_t *buf)
{
    // not a thing on ps2
}

void RockBandDrumsAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    if (m_profile->drum_state.cymbal_glitch_fix && m_mapping.has_debounce)
    {
        if (m_mapping.mapping.mapping.rbDrumAxis >= RockBandDrums_GreenPad && m_mapping.mapping.mapping.rbDrumAxis <= RockBandDrums_GreenCymbal)
        {
            if (millis() - m_profile->drum_state.last_global_poll > m_mapping.debounce) {
                m_profile->drum_state.last_drum = RockBandDrums_RedPad;
            }
            if (m_profile->drum_state.last_drum != RockBandDrums_RedPad && m_profile->drum_state.last_drum != m_mapping.mapping.mapping.rbDrumAxis)
            {
                m_last_poll = millis();
                return;
            }
            m_profile->drum_state.last_global_poll = millis();
            m_profile->drum_state.last_drum = m_mapping.mapping.mapping.rbDrumAxis;
        }
    }
    switch (m_mapping.mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        m_profile->drum_state.red_pad = m_calibrated_value;
        break;
    case RockBandDrums_YellowPad:
        m_profile->drum_state.yellow_pad = m_calibrated_value;
        break;
    case RockBandDrums_BluePad:
        m_profile->drum_state.blue_pad = m_calibrated_value;
        break;
    case RockBandDrums_GreenPad:
        m_profile->drum_state.green_pad = m_calibrated_value;
        break;
    case RockBandDrums_YellowCymbal:
        m_profile->drum_state.yellow_cymbal = m_calibrated_value;
        break;
    case RockBandDrums_BlueCymbal:
        m_profile->drum_state.blue_cymbal = m_calibrated_value;
        break;
    case RockBandDrums_GreenCymbal:
        m_profile->drum_state.green_cymbal = m_calibrated_value;
        break;
    default:
        break;
    }
    PS3RockBandDrums_Data_t *report = (PS3RockBandDrums_Data_t *)buf;
    if (m_profile->drum_state.yellow_cymbal && !m_profile->drum_state.yellow_pad)
    {
        report->yellowVelocity = 0xFF - (m_profile->drum_state.yellow_cymbal >> 8);
        report->y = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (m_profile->drum_state.yellow_pad && !m_profile->drum_state.yellow_cymbal)
    {
        report->yellowVelocity = 0xFF - (m_profile->drum_state.yellow_pad >> 8);
        report->y = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.yellow_pad && m_profile->drum_state.yellow_cymbal && !m_profile->drum_state.red_pad)
    {
        report->redVelocity = 0xFF - (m_profile->drum_state.yellow_cymbal >> 8);
        report->yellowVelocity = 0xFF - (m_profile->drum_state.yellow_cymbal >> 8);
        report->y = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (m_profile->drum_state.blue_cymbal && !m_profile->drum_state.blue_pad)
    {
        report->blueVelocity = 0xFF - (m_profile->drum_state.blue_cymbal >> 8);
        report->x = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (m_profile->drum_state.blue_pad && !m_profile->drum_state.blue_cymbal)
    {
        report->blueVelocity = 0xFF - (m_profile->drum_state.blue_pad >> 8);
        report->x = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.blue_pad && m_profile->drum_state.blue_cymbal && !m_profile->drum_state.red_pad)
    {
        report->redVelocity = 0xFF - (m_profile->drum_state.blue_cymbal >> 8);
        report->blueVelocity = 0xFF - (m_profile->drum_state.blue_pad >> 8);
        report->x = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (m_profile->drum_state.green_cymbal && !m_profile->drum_state.green_pad)
    {
        report->greenVelocity = 0xFF - (m_profile->drum_state.green_cymbal >> 8);
        report->a = true;
        report->cymbalFlag = true;
    }
    if (m_profile->drum_state.green_pad && !m_profile->drum_state.green_cymbal)
    {
        report->greenVelocity = 0xFF - (m_profile->drum_state.green_pad >> 8);
        report->a = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.green_pad && m_profile->drum_state.green_cymbal && !m_profile->drum_state.red_pad)
    {
        report->redVelocity = 0xFF - (m_profile->drum_state.green_cymbal >> 8);
        report->greenVelocity = 0xFF - (m_profile->drum_state.green_pad >> 8);
        report->a = true;
        report->padFlag = true;
        report->cymbalFlag = true;
    }
    if (m_profile->drum_state.red_pad)
    {
        report->redVelocity = 0xFF - (m_profile->drum_state.red_pad >> 8);
        report->b = true;
        report->padFlag = true;
    }
}

void RockBandDrumsAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS4RockBandDrums_Data_t *report = (PS4RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        report->redVelocity = m_calibrated_value >> 8;
        report->b = true;
        break;
    case RockBandDrums_YellowPad:
        report->yellowVelocity = m_calibrated_value >> 8;
        report->y = true;
        break;
    case RockBandDrums_BluePad:
        report->blueVelocity = m_calibrated_value >> 8;
        report->x = true;
        break;
    case RockBandDrums_GreenPad:
        report->greenVelocity = m_calibrated_value >> 8;
        report->a = true;
        break;
    case RockBandDrums_YellowCymbal:
        report->yellowCymbalVelocity = m_calibrated_value >> 8;
        report->y = true;
        break;
    case RockBandDrums_BlueCymbal:
        report->blueCymbalVelocity = m_calibrated_value >> 8;
        report->x = true;
        break;
    case RockBandDrums_GreenCymbal:
        report->greenCymbalVelocity = m_calibrated_value >> 8;
        report->a = true;
        break;
    default:
        break;
    }
}

void RockBandDrumsAxisMapping::update_ps5(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS5RockBandDrums_Data_t *report = (PS5RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        report->redVelocity = m_calibrated_value >> 8;
        report->b = true;
        break;
    case RockBandDrums_YellowPad:
        report->yellowVelocity = m_calibrated_value >> 8;
        report->y = true;
        break;
    case RockBandDrums_BluePad:
        report->blueVelocity = m_calibrated_value >> 8;
        report->x = true;
        break;
    case RockBandDrums_GreenPad:
        report->greenVelocity = m_calibrated_value >> 8;
        report->a = true;
        break;
    case RockBandDrums_YellowCymbal:
        report->yellowCymbalVelocity = m_calibrated_value >> 8;
        report->y = true;
        break;
    case RockBandDrums_BlueCymbal:
        report->blueCymbalVelocity = m_calibrated_value >> 8;
        report->x = true;
        break;
    case RockBandDrums_GreenCymbal:
        report->greenCymbalVelocity = m_calibrated_value >> 8;
        report->a = true;
        break;
    default:
        break;
    }
}

void RockBandDrumsAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    if (m_profile->drum_state.cymbal_glitch_fix && m_mapping.has_debounce)
    {
        if (m_mapping.mapping.mapping.rbDrumAxis >= RockBandDrums_GreenPad && m_mapping.mapping.mapping.rbDrumAxis <= RockBandDrums_GreenCymbal)
        {
            if (millis() - m_profile->drum_state.last_global_poll > m_mapping.debounce) {
                m_profile->drum_state.last_drum = RockBandDrums_RedPad;
            }
            if (m_profile->drum_state.last_drum != RockBandDrums_RedPad && m_profile->drum_state.last_drum != m_mapping.mapping.mapping.rbDrumAxis)
            {
                m_last_poll = millis();
                return;
            }
            m_profile->drum_state.last_global_poll = millis();
            m_profile->drum_state.last_drum = m_mapping.mapping.mapping.rbDrumAxis;
        }
    }
    switch (m_mapping.mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        m_profile->drum_state.red_pad = m_calibrated_value;
        break;
    case RockBandDrums_YellowPad:
        m_profile->drum_state.yellow_pad = m_calibrated_value;
        break;
    case RockBandDrums_BluePad:
        m_profile->drum_state.blue_pad = m_calibrated_value;
        break;
    case RockBandDrums_GreenPad:
        m_profile->drum_state.green_pad = m_calibrated_value;
        break;
    case RockBandDrums_YellowCymbal:
        m_profile->drum_state.yellow_cymbal = m_calibrated_value;
        break;
    case RockBandDrums_BlueCymbal:
        m_profile->drum_state.blue_cymbal = m_calibrated_value;
        break;
    case RockBandDrums_GreenCymbal:
        m_profile->drum_state.green_cymbal = m_calibrated_value;
        break;
    default:
        break;
    }
    XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)buf;
    if (m_profile->drum_state.yellow_cymbal && !m_profile->drum_state.yellow_pad)
    {
        report->yellowVelocity = -((m_profile->drum_state.yellow_cymbal >> 1) - 32768);
        report->y = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (m_profile->drum_state.yellow_pad && !m_profile->drum_state.yellow_cymbal)
    {
        report->yellowVelocity = -((m_profile->drum_state.yellow_pad >> 1) - 32768);
        report->y = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.yellow_pad && m_profile->drum_state.yellow_cymbal && !m_profile->drum_state.red_pad)
    {
        report->redVelocity = (m_profile->drum_state.yellow_cymbal >> 1) - 32768;
        report->yellowVelocity = -((m_profile->drum_state.yellow_cymbal >> 1) - 32768);
        report->y = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (m_profile->drum_state.blue_cymbal && !m_profile->drum_state.blue_pad)
    {
        report->blueVelocity = (m_profile->drum_state.blue_cymbal >> 1) - 32768;
        report->x = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (m_profile->drum_state.blue_pad && !m_profile->drum_state.blue_cymbal)
    {
        report->blueVelocity = (m_profile->drum_state.blue_pad >> 1) - 32768;
        report->x = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.blue_pad && m_profile->drum_state.blue_cymbal && !m_profile->drum_state.red_pad)
    {
        report->redVelocity = (m_profile->drum_state.blue_cymbal >> 1) - 32768;
        report->blueVelocity = (m_profile->drum_state.blue_pad >> 1) - 32768;
        report->x = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (m_profile->drum_state.green_cymbal && !m_profile->drum_state.green_pad)
    {
        report->greenVelocity = -((m_profile->drum_state.green_cymbal >> 1) - 32768);
        report->a = true;
        report->cymbalFlag = true;
    }
    if (m_profile->drum_state.green_pad && !m_profile->drum_state.green_cymbal)
    {
        report->greenVelocity = -((m_profile->drum_state.green_pad >> 1) - 32768);
        report->a = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.green_pad && m_profile->drum_state.green_cymbal && !m_profile->drum_state.red_pad)
    {
        report->redVelocity = (m_profile->drum_state.green_cymbal >> 1) - 32768;
        report->greenVelocity = -((m_profile->drum_state.green_pad >> 1) - 32768);
        report->a = true;
        report->padFlag = true;
        report->cymbalFlag = true;
    }
    if (m_profile->drum_state.red_pad)
    {
        report->redVelocity = (m_profile->drum_state.red_pad >> 1) - 32768;
        report->b = true;
        report->padFlag = true;
    }
}
void RockBandDrumsAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxRockBandDrums_Data_t *report = (OGXboxRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        report->redVelocity = m_calibrated_value - 32768;
        report->b = true;
        report->padFlag = true;
        break;
    case RockBandDrums_YellowPad:
        report->yellowVelocity = m_calibrated_value - 32768;
        report->y = true;
        report->padFlag = true;
        break;
    case RockBandDrums_BluePad:
        report->blueVelocity = m_calibrated_value - 32768;
        report->x = true;
        report->padFlag = true;
        break;
    case RockBandDrums_GreenPad:
        report->greenVelocity = m_calibrated_value - 32768;
        report->a = true;
        report->padFlag = true;
        break;
    case RockBandDrums_YellowCymbal:
        report->yellowVelocity = m_calibrated_value - 32768;
        report->y = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
        break;
    case RockBandDrums_BlueCymbal:
        report->blueVelocity = m_calibrated_value - 32768;
        report->x = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
        break;
    case RockBandDrums_GreenCymbal:
        report->greenVelocity = m_calibrated_value - 32768;
        report->a = true;
        report->cymbalFlag = true;
        break;
    default:
        break;
    }
}
void RockBandDrumsAxisMapping::update_xboxone(uint8_t *buf)
{

    if (m_centered)
    {
        return;
    }
    XboxOneRockBandDrums_Data_t *report = (XboxOneRockBandDrums_Data_t *)buf;

    switch (m_mapping.mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        report->redVelocity = m_calibrated_value >> 12;
        report->b = true;
        break;
    case RockBandDrums_YellowPad:
        report->yellowVelocity = m_calibrated_value >> 12;
        report->y = true;
        break;
    case RockBandDrums_BluePad:
        report->blueVelocity = m_calibrated_value >> 12;
        report->x = true;
        break;
    case RockBandDrums_GreenPad:
        report->greenVelocity = m_calibrated_value >> 12;
        report->a = true;
        break;
    case RockBandDrums_YellowCymbal:
        report->yellowCymbalVelocity = m_calibrated_value >> 12;
        report->y = true;
        break;
    case RockBandDrums_BlueCymbal:
        report->blueCymbalVelocity = m_calibrated_value >> 12;
        report->x = true;
        break;
    case RockBandDrums_GreenCymbal:
        report->greenCymbalVelocity = m_calibrated_value >> 12;
        report->a = true;
        break;
    // case RockBandDrums_LeftStickX:
    //     if (!m_centered)
    //     {
    //         report->redVelocity = m_calibrated_value - 32768;
    //     }
    //     break;
    // case RockBandDrums_LeftStickY:
    //     if (!m_centered)
    //     {
    //         report->yellowVelocity = m_calibrated_value - 32768;
    //     }
    //     break;
    default:
        break;
    }
}
RockBandDrumsGamepadAxisMapping::~RockBandDrumsGamepadAxisMapping() {}

RockBandDrumsGamepadAxisMapping::RockBandDrumsGamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadAxisMapping(mapping, std::move(input), id, profile)
{
}
void RockBandDrumsGamepadAxisMapping::update_xboxone(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    (void)buf;
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
        // report->joystickX = m_calibrated_value - 32768;
        break;
    case Gamepad_LeftStickY:
        // report->joystickY = m_calibrated_value - 32768;
        break;
    case Gamepad_RightStickX:
        // report->rightStickX = m_calibrated_value - 32768;
        break;
    case Gamepad_RightStickY:
        // report->rightStickY = m_calibrated_value - 32768;
        break;
    case Gamepad_LeftTrigger:
        // report->leftTrigger = m_calibrated_value >> 6;
        break;
    case Gamepad_RightTrigger:
        // report->rightTrigger = m_calibrated_value >> 6;
        break;
    default:
        break;
    }
}

ProGuitarButtonMapping::ProGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void ProGuitarButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void ProGuitarButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void ProGuitarButtonMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void ProGuitarButtonMapping::update_ps2(uint8_t *buf)
{
    // not a thing
}

void ProGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3RockBandProGuitar_Data_t *report = (PS3RockBandProGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proButton)
    {
    case ProGuitar_Green:
        report->green |= m_last_value;
        break;
    case ProGuitar_Red:
        report->red |= m_last_value;
        break;
    case ProGuitar_Yellow:
        report->yellow |= m_last_value;
        break;
    case ProGuitar_Blue:
        report->blue |= m_last_value;
        break;
    case ProGuitar_Orange:
        report->orange |= m_last_value;
        break;
    case ProGuitar_SoloGreen:
        report->green |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_SoloRed:
        report->red |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_SoloYellow:
        report->yellow |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_SoloBlue:
        report->blue |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_SoloOrange:
        report->orange |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_Pedal:
        report->pedal |= m_last_value;
        break;
    default:
        break;
    }
}

void ProGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    // not a thing
}

void ProGuitarButtonMapping::update_ps5(uint8_t *buf)
{
    // not a thing
}

void ProGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandProGuitar_Data_t *report = (XInputRockBandProGuitar_Data_t *)buf;
    
    switch (m_mapping.mapping.mapping.proButton)
    {
    case ProGuitar_Green:
        report->green |= m_last_value;
        break;
    case ProGuitar_Red:
        report->red |= m_last_value;
        break;
    case ProGuitar_Yellow:
        report->yellow |= m_last_value;
        break;
    case ProGuitar_Blue:
        report->blue |= m_last_value;
        break;
    case ProGuitar_Orange:
        report->orange |= m_last_value;
        break;
    case ProGuitar_SoloGreen:
        report->green |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_SoloRed:
        report->red |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_SoloYellow:
        report->yellow |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_SoloBlue:
        report->blue |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_SoloOrange:
        report->orange |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_Pedal:
        report->pedal |= m_last_value;
        break;
    default:
        break;
    }
}
void ProGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandProGuitar_Data_t *report = (OGXboxRockBandProGuitar_Data_t *)buf;
    
    switch (m_mapping.mapping.mapping.proButton)
    {
    case ProGuitar_Green:
        report->green |= m_last_value;
        break;
    case ProGuitar_Red:
        report->red |= m_last_value;
        break;
    case ProGuitar_Yellow:
        report->yellow |= m_last_value;
        break;
    case ProGuitar_Blue:
        report->blue |= m_last_value;
        break;
    case ProGuitar_Orange:
        report->orange |= m_last_value;
        break;
    case ProGuitar_SoloGreen:
        report->green |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_SoloRed:
        report->red |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_SoloYellow:
        report->yellow |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_SoloBlue:
        report->blue |= m_last_value;
        report->solo |= m_last_value;
        break;
    case ProGuitar_SoloOrange:
        report->orange |= m_last_value;
        report->solo |= m_last_value;
        break;
    default:
        break;
    }
}
void ProGuitarButtonMapping::update_xboxone(uint8_t *buf)
{
}

ProGuitarAxisMapping::ProGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.proAxis != ProGuitar_Tilt)
{
}

void ProGuitarAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void ProGuitarAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void ProGuitarAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void ProGuitarAxisMapping::update_ps2(uint8_t *buf)
{
    // not a thing
}

void ProGuitarAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS3RockBandProGuitar_Data_t *report = (PS3RockBandProGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proAxis)
    {
    case ProGuitar_LowEFret:
        report->lowEFret = m_calibrated_value;
        break;
    case ProGuitar_AFret:
        report->aFret = m_calibrated_value;
        break;
    case ProGuitar_DFret:
        report->dFret = m_calibrated_value;
        break;
    case ProGuitar_GFret:
        report->gFret = m_calibrated_value;
        break;
    case ProGuitar_BFret:
        report->bFret = m_calibrated_value;
        break;
    case ProGuitar_HighEFret:
        report->highEFret = m_calibrated_value;
        break;
    case ProGuitar_LowEFretVelocity:
        report->lowEFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_AFretVelocity:
        report->aFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_DFretVelocity:
        report->dFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_GFretVelocity:
        report->gFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_BFretVelocity:
        report->bFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_HighEFretVelocity:
        report->highEFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_Tilt:
        report->tilt = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}

void ProGuitarAxisMapping::update_ps4(uint8_t *buf)
{
}

void ProGuitarAxisMapping::update_ps5(uint8_t *buf)
{
}

void ProGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputRockBandProGuitar_Data_t *report = (XInputRockBandProGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proAxis)
    {
    case ProGuitar_LowEFret:
        report->lowEFret = m_calibrated_value;
        break;
    case ProGuitar_AFret:
        report->aFret = m_calibrated_value;
        break;
    case ProGuitar_DFret:
        report->dFret = m_calibrated_value;
        break;
    case ProGuitar_GFret:
        report->gFret = m_calibrated_value;
        break;
    case ProGuitar_BFret:
        report->bFret = m_calibrated_value;
        break;
    case ProGuitar_HighEFret:
        report->highEFret = m_calibrated_value;
        break;
    case ProGuitar_LowEFretVelocity:
        report->lowEFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_AFretVelocity:
        report->aFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_DFretVelocity:
        report->dFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_GFretVelocity:
        report->gFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_BFretVelocity:
        report->bFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_HighEFretVelocity:
        report->highEFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_Tilt:
        report->tilt = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}
void ProGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxRockBandProGuitar_Data_t *report = (OGXboxRockBandProGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proAxis)
    {
    case ProGuitar_LowEFret:
        report->lowEFret = m_calibrated_value >> 8;
        break;
    case ProGuitar_AFret:
        report->aFret = m_calibrated_value >> 8;
        break;
    case ProGuitar_DFret:
        report->dFret = m_calibrated_value >> 8;
        break;
    case ProGuitar_GFret:
        report->gFret = m_calibrated_value >> 8;
        break;
    case ProGuitar_BFret:
        report->bFret = m_calibrated_value >> 8;
        break;
    case ProGuitar_HighEFret:
        report->highEFret = m_calibrated_value >> 8;
        break;
    case ProGuitar_LowEFretVelocity:
        report->lowEFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_AFretVelocity:
        report->aFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_DFretVelocity:
        report->dFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_GFretVelocity:
        report->gFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_BFretVelocity:
        report->bFretVelocity = m_calibrated_value >> 8;
        break;
    case ProGuitar_HighEFretVelocity:
        report->highEFretVelocity = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}
void ProGuitarAxisMapping::update_xboxone(uint8_t *buf)
{
}

ProKeysButtonMapping::ProKeysButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void ProKeysButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
    
}
void ProKeysButtonMapping::update_wii(uint8_t format, uint8_t *buf)
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

void ProKeysButtonMapping::update_ps5(uint8_t *buf)
{
    
}

void ProKeysButtonMapping::update_xinput(uint8_t *buf)
{
    
}
void ProKeysButtonMapping::update_ogxbox(uint8_t *buf)
{
    
}
void ProKeysButtonMapping::update_xboxone(uint8_t *buf)
{
}

ProKeysAxisMapping::ProKeysAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}

void ProKeysAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void ProKeysAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
}
void ProKeysAxisMapping::update_switch(uint8_t *buf)
{
}

void ProKeysAxisMapping::update_ps2(uint8_t *buf)
{
}

void ProKeysAxisMapping::update_ps3(uint8_t *buf)
{
}

void ProKeysAxisMapping::update_ps4(uint8_t *buf)
{
}

void ProKeysAxisMapping::update_ps5(uint8_t *buf)
{
}

void ProKeysAxisMapping::update_xinput(uint8_t *buf)
{
}
void ProKeysAxisMapping::update_ogxbox(uint8_t *buf)
{
}
void ProKeysAxisMapping::update_xboxone(uint8_t *buf)
{
}
