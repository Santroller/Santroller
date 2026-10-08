#include <string.h>
#include "protocols/rb_pickup.hpp"
#include "events.pb.h"
#include "instance.hpp"
#include "main.hpp"
#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "emulation/usb/hid_device.h"
#include "input/midi.hpp"
#include <pb_encode.h>
#include <stdint.h>
#include <utils.h>
#include <math.h>

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
        report->green |= m_last_value;
        break;
    case RockBandGuitar_Red:
        report->b |= m_last_value;
        report->red |= m_last_value;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_last_value;
        report->yellow |= m_last_value;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_last_value;
        report->blue |= m_last_value;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_last_value;
        report->orange |= m_last_value;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_last_value;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_last_value;
        report->solo |= m_last_value;
        report->soloGreen |= m_last_value;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_last_value;
        report->solo |= m_last_value;
        report->soloRed |= m_last_value;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_last_value;
        report->solo |= m_last_value;
        report->soloYellow |= m_last_value;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_last_value;
        report->solo |= m_last_value;
        report->soloBlue |= m_last_value;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_last_value;
        report->solo |= m_last_value;
        report->soloOrange |= m_last_value;
        break;
    }
}

// The pickup is calibrated like a trigger, so min / max maps linearly onto the full range
// rather than being split around a centre point like a stick
RockBandGuitarAxisMapping::RockBandGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.rbAxis == RockBandGuitar_Whammy || mapping.mapping.mapping.rbAxis == RockBandGuitar_Pickup)
{
    if (mapping.pickupThresholds_count == 4)
    {
        m_has_pickup_thresholds = true;
        memcpy(m_pickup_thresholds, mapping.pickupThresholds, sizeof(m_pickup_thresholds));
    }
}

uint8_t RockBandGuitarAxisMapping::pickup_notch() const
{
    if (!m_has_pickup_thresholds)
    {
        // equal bands, which also lines up with the cycle device's pickup values
        uint32_t notch = m_calibrated_value * 5 / 65536;
        return notch > 4 ? 4 : notch;
    }
    for (uint8_t i = 0; i < 4; i++)
    {
        if (m_calibrated_value <= m_pickup_thresholds[i])
        {
            return i;
        }
    }
    return 4;
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
    if (m_centered && !is_pickup())
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
        report->pickup = rb_pickup_universal[pickup_notch()];
        break;
    }
}

void RockBandGuitarAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered && !is_pickup())
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
        report->tilt = abs(int32_t(m_calibrated_value - 32768)) >> 7;
        break;
    case RockBandGuitar_Pickup:
        // RB4 guitars report the notch directly, 0 - 4
        report->pickup = pickup_notch();
        break;
    default:
        break;
    }
}

void RockBandGuitarAxisMapping::update_ps5(uint8_t *buf)
{
    if (m_centered && !is_pickup())
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
        report->tilt = abs(int32_t(m_calibrated_value - 32768)) >> 7;
        break;
    case RockBandGuitar_Pickup:
        // RB4 guitars report the notch directly, 0 - 4
        report->pickup = pickup_notch();
        break;
    default:
        break;
    }
}

void RockBandGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered && !is_pickup())
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
        report->pickup = rb_pickup_universal[pickup_notch()];
        break;
    default:
        break;
    }
}
void RockBandGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered && !is_pickup())
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
        report->pickup = rb_pickup_universal[pickup_notch()];
        break;
    default:
        break;
    }
}
void RockBandGuitarAxisMapping::update_xboxone(uint8_t *buf)
{
    if (m_centered && !is_pickup())
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
        report->pickup = rb_pickup_xbox_one[pickup_notch()];
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

static bool is_rock_band_drum_cymbal(RockBandDrumsAxisType axis)
{
    return axis == RockBandDrums_YellowCymbal || axis == RockBandDrums_BlueCymbal || axis == RockBandDrums_GreenCymbal;
}

// Rock Band can't take two cymbals at once (the cymbal glitch), or the green pad and green
// cymbal together (the lefty glitch), so those are staggered with a gap between them. A hit
// that has to wait stays held, so it's shown for a whole debounce once it's let through.
bool RockBandDrumsAxisMapping::should_emit_cymbal_hit(RockBandDrumsAxisType axis)
{
    if (!m_profile->cymbal_glitch_fix)
    {
        return true;
    }
    auto &state = m_profile->drum_state;
    const uint64_t now = time_us_64();
    const uint32_t debounce = m_mapping.has_debounce ? m_mapping.debounce_us : 25000;
    const bool cymbal_gap = now - state.last_cymbal_off > debounce;
    const bool green_gap = now - state.last_green_off > debounce;
    bool allowed;
    switch (axis)
    {
    case RockBandDrums_GreenPad:
        allowed = !state.green_cymbal_on && green_gap;
        break;
    case RockBandDrums_GreenCymbal:
        allowed = !state.yellow_cymbal_on && !state.blue_cymbal_on && !state.green_pad_on && green_gap && cymbal_gap;
        break;
    case RockBandDrums_BlueCymbal:
        allowed = !state.green_cymbal_on && !state.yellow_cymbal_on && cymbal_gap;
        break;
    case RockBandDrums_YellowCymbal:
        allowed = !state.green_cymbal_on && !state.blue_cymbal_on && cymbal_gap;
        break;
    default:
        return true;
    }
    if (!allowed)
    {
        m_last_poll = time_us_64();
        return false;
    }
    switch (axis)
    {
    case RockBandDrums_GreenPad:
        state.green_pad_on = true;
        break;
    case RockBandDrums_GreenCymbal:
        state.green_cymbal_on = true;
        break;
    case RockBandDrums_BlueCymbal:
        state.blue_cymbal_on = true;
        break;
    case RockBandDrums_YellowCymbal:
        state.yellow_cymbal_on = true;
        break;
    default:
        break;
    }
    return true;
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
    auto axis = m_mapping.mapping.mapping.rbDrumAxis;
    auto calibrated_value = m_calibrated_value;
    if (m_centered || !should_emit_cymbal_hit(axis))
    {
        return;
    }
    if (is_rock_band_drum_cymbal(axis))
    {
        m_profile->drum_state.cymbal_this_report = true;
    }
    else
    {
        m_profile->drum_state.pad_this_report = true;
    }
    switch (axis)
    {
    case RockBandDrums_RedPad:
        m_profile->drum_state.red_pad = calibrated_value;
        break;
    case RockBandDrums_YellowPad:
        m_profile->drum_state.yellow_pad = calibrated_value;
        break;
    case RockBandDrums_BluePad:
        m_profile->drum_state.blue_pad = calibrated_value;
        break;
    case RockBandDrums_GreenPad:
        m_profile->drum_state.green_pad = calibrated_value;
        break;
    case RockBandDrums_YellowCymbal:
        m_profile->drum_state.yellow_cymbal = calibrated_value;
        break;
    case RockBandDrums_BlueCymbal:
        m_profile->drum_state.blue_cymbal = calibrated_value;
        break;
    case RockBandDrums_GreenCymbal:
        m_profile->drum_state.green_cymbal = calibrated_value;
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
        report->yellowVelocity = 0xFF - (m_profile->drum_state.yellow_pad >> 8);
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

    if (report->dpadUp && report->dpadDown)
    {
        if (m_profile->drum_state.yellow_cymbal >= m_profile->drum_state.blue_cymbal)
        {
            report->dpadDown = false;
        }
        else
        {
            report->dpadUp = false;
        }
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
    auto axis = m_mapping.mapping.mapping.rbDrumAxis;
    auto calibrated_value = m_calibrated_value;
    if (m_centered || !should_emit_cymbal_hit(axis))
    {
        return;
    }
    if (is_rock_band_drum_cymbal(axis))
    {
        m_profile->drum_state.cymbal_this_report = true;
    }
    else
    {
        m_profile->drum_state.pad_this_report = true;
    }
    switch (axis)
    {
    case RockBandDrums_RedPad:
        m_profile->drum_state.red_pad = calibrated_value;
        break;
    case RockBandDrums_YellowPad:
        m_profile->drum_state.yellow_pad = calibrated_value;
        break;
    case RockBandDrums_BluePad:
        m_profile->drum_state.blue_pad = calibrated_value;
        break;
    case RockBandDrums_GreenPad:
        m_profile->drum_state.green_pad = calibrated_value;
        break;
    case RockBandDrums_YellowCymbal:
        m_profile->drum_state.yellow_cymbal = calibrated_value;
        break;
    case RockBandDrums_BlueCymbal:
        m_profile->drum_state.blue_cymbal = calibrated_value;
        break;
    case RockBandDrums_GreenCymbal:
        m_profile->drum_state.green_cymbal = calibrated_value;
        break;
    default:
        break;
    }
    XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)buf;
    if (m_profile->drum_state.yellow_cymbal && !m_profile->drum_state.yellow_pad)
    {
        report->yellowVelocity = -(32768 - (m_profile->drum_state.yellow_cymbal >> 1));
        report->y = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (m_profile->drum_state.yellow_pad && !m_profile->drum_state.yellow_cymbal)
    {
        report->yellowVelocity = -(32768 - (m_profile->drum_state.yellow_pad >> 1));
        report->y = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.yellow_pad && m_profile->drum_state.yellow_cymbal && !m_profile->drum_state.red_pad)
    {
        report->redVelocity = (32768 - (m_profile->drum_state.yellow_cymbal >> 1));
        report->yellowVelocity = -(32768 - (m_profile->drum_state.yellow_pad >> 1));
        report->y = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadUp = true;
    }
    if (m_profile->drum_state.blue_cymbal && !m_profile->drum_state.blue_pad)
    {
        report->blueVelocity = (32768 - (m_profile->drum_state.blue_cymbal >> 1));
        report->x = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (m_profile->drum_state.blue_pad && !m_profile->drum_state.blue_cymbal)
    {
        report->blueVelocity = (32768 - (m_profile->drum_state.blue_pad >> 1));
        report->x = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.blue_pad && m_profile->drum_state.blue_cymbal && !m_profile->drum_state.red_pad)
    {
        report->redVelocity = (32768 - (m_profile->drum_state.blue_cymbal >> 1));
        report->blueVelocity = (32768 - (m_profile->drum_state.blue_pad >> 1));
        report->x = true;
        report->padFlag = true;
        report->cymbalFlag = true;
        report->dpadDown = true;
    }
    if (m_profile->drum_state.green_cymbal && !m_profile->drum_state.green_pad)
    {
        report->greenVelocity = -(32768 - (m_profile->drum_state.green_cymbal >> 1));
        report->a = true;
        report->cymbalFlag = true;
    }
    if (m_profile->drum_state.green_pad && !m_profile->drum_state.green_cymbal)
    {
        report->greenVelocity = -(32768 - (m_profile->drum_state.green_pad >> 1));
        report->a = true;
        report->padFlag = true;
    }
    if (m_profile->drum_state.green_pad && m_profile->drum_state.green_cymbal && !m_profile->drum_state.red_pad)
    {
        report->redVelocity = (32768 - (m_profile->drum_state.green_cymbal >> 1));
        report->greenVelocity = -(32768 - (m_profile->drum_state.green_pad >> 1));
        report->a = true;
        report->padFlag = true;
        report->cymbalFlag = true;
    }
    if (m_profile->drum_state.red_pad)
    {
        report->redVelocity = (32768 - (m_profile->drum_state.red_pad >> 1));
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
    // Velocity is 4 bits; a light hit must never round to 0 or the game reads it as the other pad type
    uint8_t velocity = m_calibrated_value >> 12;
    if (velocity == 0)
    {
        velocity = 1;
    }

    switch (m_mapping.mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        report->redVelocity = velocity;
        report->b = true;
        break;
    case RockBandDrums_YellowPad:
        report->yellowVelocity = velocity;
        report->y = true;
        break;
    case RockBandDrums_BluePad:
        report->blueVelocity = velocity;
        report->x = true;
        break;
    case RockBandDrums_GreenPad:
        report->greenVelocity = velocity;
        report->a = true;
        break;
    case RockBandDrums_YellowCymbal:
        report->yellowCymbalVelocity = velocity;
        report->y = true;
        break;
    case RockBandDrums_BlueCymbal:
        report->blueCymbalVelocity = velocity;
        report->x = true;
        break;
    case RockBandDrums_GreenCymbal:
        report->greenCymbalVelocity = velocity;
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
bool DrumsGamepadAxisMapping::is_stick() const
{
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:
    case Gamepad_LeftStickY:
    case Gamepad_RightStickX:
    case Gamepad_RightStickY:
        return true;
    default:
        return false;
    }
}
void DrumsGamepadAxisMapping::update_hid(uint8_t *buf)
{
    if (!is_stick())
    {
        GamepadAxisMapping::update_hid(buf);
    }
}
void DrumsGamepadAxisMapping::update_xinput(uint8_t *buf)
{
    if (!is_stick())
    {
        GamepadAxisMapping::update_xinput(buf);
    }
}
void DrumsGamepadAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (!is_stick())
    {
        GamepadAxisMapping::update_ogxbox(buf);
    }
}

RockBandDrumsGamepadAxisMapping::~RockBandDrumsGamepadAxisMapping() {}

RockBandDrumsGamepadAxisMapping::RockBandDrumsGamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : DrumsGamepadAxisMapping(mapping, std::move(input), id, profile)
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
        report->pedalConnection = true;
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
        report->soloFlag |= m_last_value;
        break;
    case ProGuitar_SoloRed:
        report->red |= m_last_value;
        report->soloFlag |= m_last_value;
        break;
    case ProGuitar_SoloYellow:
        report->yellow |= m_last_value;
        report->soloFlag |= m_last_value;
        break;
    case ProGuitar_SoloBlue:
        report->blue |= m_last_value;
        report->soloFlag |= m_last_value;
        break;
    case ProGuitar_SoloOrange:
        report->orange |= m_last_value;
        report->soloFlag |= m_last_value;
        break;
    case ProGuitar_Pedal:
        report->pedal |= m_last_value;
        report->pedalConnection = true;
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
    // Pro guitars don't exist on Xbox One, so report as a standard Rock Band guitar
    XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proButton)
    {
    case ProGuitar_Green:
        report->a |= m_last_value;
        report->green |= m_last_value;
        break;
    case ProGuitar_Red:
        report->b |= m_last_value;
        report->red |= m_last_value;
        break;
    case ProGuitar_Yellow:
        report->y |= m_last_value;
        report->yellow |= m_last_value;
        break;
    case ProGuitar_Blue:
        report->x |= m_last_value;
        report->blue |= m_last_value;
        break;
    case ProGuitar_Orange:
        report->leftShoulder |= m_last_value;
        report->orange |= m_last_value;
        break;
    case ProGuitar_Pedal:
        report->rightShoulder |= m_last_value;
        break;
    case ProGuitar_SoloGreen:
        report->a |= m_last_value;
        report->solo |= m_last_value;
        report->soloGreen |= m_last_value;
        break;
    case ProGuitar_SoloRed:
        report->b |= m_last_value;
        report->solo |= m_last_value;
        report->soloRed |= m_last_value;
        break;
    case ProGuitar_SoloYellow:
        report->y |= m_last_value;
        report->solo |= m_last_value;
        report->soloYellow |= m_last_value;
        break;
    case ProGuitar_SoloBlue:
        report->x |= m_last_value;
        report->solo |= m_last_value;
        report->soloBlue |= m_last_value;
        break;
    case ProGuitar_SoloOrange:
        report->leftShoulder |= m_last_value;
        report->solo |= m_last_value;
        report->soloOrange |= m_last_value;
        break;
    default:
        break;
    }
}

static uint8_t pro_guitar_tilt_report_value(uint16_t value)
{
    const uint32_t report_value = (uint32_t(value) + 0x100) >> 9;
    return static_cast<uint8_t>(report_value > 0x7f ? 0x7f : report_value);
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
    PS3RockBandProGuitar_Data_t *report = (PS3RockBandProGuitar_Data_t *)buf;
    if (m_mapping.mapping.mapping.proAxis == ProGuitar_Tilt)
    {
        const uint8_t tilt = pro_guitar_tilt_report_value(m_calibrated_value);
        report->autoCal_Microphone = tilt;
        report->autoCal_Light = tilt;
        report->tilt = tilt;
        return;
    }
    if (m_centered)
    {
        return;
    }
    switch (m_mapping.mapping.mapping.proAxis)
    {
    case ProGuitar_LowEFret:
        report->lowEFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_AFret:
        report->aFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_DFret:
        report->dFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_GFret:
        report->gFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_BFret:
        report->bFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_HighEFret:
        report->highEFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_LowEFretVelocity:
        report->lowEFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_AFretVelocity:
        report->aFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_DFretVelocity:
        report->dFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_GFretVelocity:
        report->gFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_BFretVelocity:
        report->bFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_HighEFretVelocity:
        report->highEFretVelocity = m_calibrated_value >> 9;
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
    XInputRockBandProGuitar_Data_t *report = (XInputRockBandProGuitar_Data_t *)buf;
    if (m_mapping.mapping.mapping.proAxis == ProGuitar_Tilt)
    {
        const uint8_t tilt = pro_guitar_tilt_report_value(m_calibrated_value);
        report->autoCal_Microphone = tilt;
        report->autoCal_Light = tilt;
        report->tilt = tilt;
        return;
    }
    if (m_centered)
    {
        return;
    }
    switch (m_mapping.mapping.mapping.proAxis)
    {
    case ProGuitar_LowEFret:
        report->lowEFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_AFret:
        report->aFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_DFret:
        report->dFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_GFret:
        report->gFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_BFret:
        report->bFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_HighEFret:
        report->highEFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_LowEFretVelocity:
        report->lowEFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_AFretVelocity:
        report->aFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_DFretVelocity:
        report->dFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_GFretVelocity:
        report->gFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_BFretVelocity:
        report->bFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_HighEFretVelocity:
        report->highEFretVelocity = m_calibrated_value >> 9;
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
        report->lowEFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_AFret:
        report->aFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_DFret:
        report->dFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_GFret:
        report->gFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_BFret:
        report->bFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_HighEFret:
        report->highEFret = m_calibrated_value >> 11;
        break;
    case ProGuitar_LowEFretVelocity:
        report->lowEFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_AFretVelocity:
        report->aFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_DFretVelocity:
        report->dFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_GFretVelocity:
        report->gFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_BFretVelocity:
        report->bFretVelocity = m_calibrated_value >> 9;
        break;
    case ProGuitar_HighEFretVelocity:
        report->highEFretVelocity = m_calibrated_value >> 9;
        break;
    default:
        break;
    }
}
void ProGuitarAxisMapping::update_xboxone(uint8_t *buf)
{
    // Pro guitars don't exist on Xbox One, so only tilt carries over to the standard Rock Band guitar
    if (m_centered || m_mapping.mapping.mapping.proAxis != ProGuitar_Tilt)
    {
        return;
    }
    XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)buf;
    report->tilt = m_calibrated_value >> 8;
}

static inline void set_keyboard_key(uint8_t &key1, uint8_t &key2, uint8_t &key3, uint8_t *velocities, uint8_t key, uint8_t velocity)
{
    if (key < 1 || key > 25)
    {
        return;
    }
    if (key <= 8)
    {
        key1 |= (1 << (8 - key));
    }
    else if (key <= 16)
    {
        key2 |= (1 << (16 - key));
    }
    else if (key <= 24)
    {
        key3 |= (1 << (24 - key));
    }
    else if (key == 25)
    {
        velocities[0] |= 0x80;
    }

    if (velocity > 0)
    {
        uint8_t vel = velocity > 127 ? 127 : velocity;
        for (int i = 0; i < 5; i++)
        {
            if ((velocities[i] & 0x7F) == 0)
            {
                if (i == 0)
                {
                    velocities[0] = (velocities[0] & 0x80) | vel;
                }
                else
                {
                    velocities[i] = vel;
                }
                break;
            }
        }
    }
}

ProKeysButtonMapping::ProKeysButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void ProKeysButtonMapping::update_hid(uint8_t *buf)
{
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
    PS3RockBandProKeyboard_Data_t *report = (PS3RockBandProKeyboard_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proKeyboardButton)
    {
    case ProKeyboardOverdrive:
        report->overdrive |= m_last_value;
        break;
    default:
        break;
    }
}
void ProKeysButtonMapping::update_ps4(uint8_t *buf)
{
}
void ProKeysButtonMapping::update_ps5(uint8_t *buf)
{
}
void ProKeysButtonMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandKeyboard_Data_t *report = (XInputRockBandKeyboard_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proKeyboardButton)
    {
    case ProKeyboardOverdrive:
        report->overdrive |= m_last_value;
        break;
    default:
        break;
    }
}
void ProKeysButtonMapping::update_ogxbox(uint8_t *buf)
{
}
void ProKeysButtonMapping::update_xboxone(uint8_t *buf)
{
}

ProKeysAxisMapping::ProKeysAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.proKeyboardAxis == ProKeyboardPedal)
{
}

void ProKeysAxisMapping::update_hid(uint8_t *buf)
{
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
    PS3RockBandProKeyboard_Data_t *report = (PS3RockBandProKeyboard_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proKeyboardAxis)
    {
    case ProKeyboardPedal:
        report->pedalAnalog = 0x7f - (m_calibrated_value >> 9);
        report->pedalDigital = m_calibrated_value != 0;
        report->pedalConnection = true;
        break;
    case ProKeyboardTouchPad:
        report->touchPad = m_calibrated_value >> 9;
        break;
    default:
        break;
    }
}
void ProKeysAxisMapping::update_ps4(uint8_t *buf)
{
}
void ProKeysAxisMapping::update_ps5(uint8_t *buf)
{
}
void ProKeysAxisMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandKeyboard_Data_t *report = (XInputRockBandKeyboard_Data_t *)buf;
    switch (m_mapping.mapping.mapping.proKeyboardAxis)
    {
    case ProKeyboardPedal:
        report->pedalAnalog = 0x7f - (m_calibrated_value >> 9);
        report->pedalDigital = m_calibrated_value != 0;
        report->pedalConnection = true;
        break;
    case ProKeyboardTouchPad:
        report->touchPad = m_calibrated_value >> 9;
        break;
    default:
        break;
    }
}
void ProKeysAxisMapping::update_ogxbox(uint8_t *buf)
{
}
void ProKeysAxisMapping::update_xboxone(uint8_t *buf)
{
}

ProKeysKeyMapping::ProKeysKeyMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile)
    : Mapping(mapping, std::move(input), id, profile)
{
    m_is_multiple = (mapping.mapping.which_mapping == proto_Output_proKeyMultiple_tag);
}

void ProKeysKeyMapping::update(bool full_poll, bool send_events)
{
    if (m_is_multiple)
    {
        m_active_keys = 0;
        int count = m_mapping.mapping.mapping.proKeyMultiple;
        if (count < 0)
        {
            count = 0;
        }
        if (count > 25)
        {
            count = 25;
        }
        auto midi = m_input->as_midi_note();
        if (midi && midi->device())
        {
            uint8_t channel = midi->channel() - 1;
            uint8_t root_note = midi->note();
            for (int i = 0; i < count; i++)
            {
                uint8_t vel = midi->device()->read_midi_note(channel, root_note + i);
                if (vel > 0)
                {
                    m_active_keys |= (1 << i);
                    m_key_velocities[i] = vel;
                }
                else
                {
                    m_key_velocities[i] = 0;
                }
            }
        }
        else if (!m_input->tick_pro_key_range(m_active_keys, m_key_velocities, count))
        {
            bool pressed = m_input->tick_digital();
            if (pressed)
            {
                m_active_keys |= 1;
                m_key_velocities[0] = m_input->tick_analog() >> 9;
            }
            else
            {
                m_key_velocities[0] = 0;
            }
        }

        bool any_pressed = m_active_keys != 0;
        if (send_events && (any_pressed != m_last_sent_pressed || full_poll))
        {
            proto_Event event = {which_event : proto_Event_button_tag, event : {button : {m_id, any_pressed, any_pressed}}};
            HIDConfigDevice::send_event(event, false);
            m_last_sent_pressed = any_pressed;
        }
    }
    else
    {
        bool pressed = false;
        uint8_t vel = 0;
        auto midi = m_input->as_midi_note();
        if (midi && midi->device())
        {
            uint8_t channel = midi->channel() - 1;
            uint8_t note = midi->note();
            vel = midi->device()->read_midi_note(channel, note);
            pressed = (vel > 0);
        }
        else
        {
            const bool input_pressed = m_input->tick_digital();
            pressed = input_pressed;
            if (input_pressed)
            {
                vel = m_input->tick_analog() >> 9;
            }
            if (m_mapping.inverted)
            {
                pressed = !pressed;
            }
            if (pressed && !input_pressed)
            {
                vel = 127;
            }
        }

        if (m_mapping.has_debounce)
        {
            if (pressed)
            {
                m_last_poll = time_us_64();
                m_single_pressed = pressed;
            }
            else if ((time_us_64() - m_last_poll) > m_mapping.debounce_us)
            {
                m_single_pressed = pressed;
            }
        }
        else
        {
            m_single_pressed = pressed;
        }

        m_single_velocity = m_single_pressed ? vel : 0;

        if (send_events && (m_single_pressed != m_last_sent_pressed || full_poll))
        {
            proto_Event event = {which_event : proto_Event_button_tag, event : {button : {m_id, m_single_pressed, m_single_pressed}}};
            HIDConfigDevice::send_event(event, false);
            m_last_sent_pressed = m_single_pressed;
        }
    }
}

void ProKeysKeyMapping::update_hid(uint8_t *buf)
{
    return update_xinput(buf);
}
void ProKeysKeyMapping::update_wii(uint8_t format, uint8_t *buf)
{
}
void ProKeysKeyMapping::update_switch(uint8_t *report)
{
}
void ProKeysKeyMapping::update_ps2(uint8_t *report)
{
}
void ProKeysKeyMapping::update_ps3(uint8_t *buf)
{
    PS3RockBandProKeyboard_Data_t *report = (PS3RockBandProKeyboard_Data_t *)buf;
    if (m_is_multiple)
    {
        for (int i = 0; i < 25; i++)
        {
            if (m_active_keys & (1 << i))
            {
                set_keyboard_key(report->key1, report->key2, report->key3, report->velocities, i + 1, m_key_velocities[i]);
            }
        }
    }
    else
    {
        if (m_single_pressed)
        {
            set_keyboard_key(report->key1, report->key2, report->key3, report->velocities, m_mapping.mapping.mapping.proKeySingle, m_single_velocity);
        }
    }
}
void ProKeysKeyMapping::update_ps4(uint8_t *report)
{
}
void ProKeysKeyMapping::update_ps5(uint8_t *report)
{
}
void ProKeysKeyMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandKeyboard_Data_t *report = (XInputRockBandKeyboard_Data_t *)buf;
    if (m_is_multiple)
    {
        for (int i = 0; i < 25; i++)
        {
            if (m_active_keys & (1 << i))
            {
                set_keyboard_key(report->key1, report->key2, report->key3, report->velocities, i + 1, m_key_velocities[i]);
            }
        }
    }
    else
    {
        if (m_single_pressed)
        {
            set_keyboard_key(report->key1, report->key2, report->key3, report->velocities, m_mapping.mapping.mapping.proKeySingle, m_single_velocity);
        }
    }
}
void ProKeysKeyMapping::update_ogxbox(uint8_t *report)
{
}
void ProKeysKeyMapping::update_xboxone(uint8_t *report)
{
}
