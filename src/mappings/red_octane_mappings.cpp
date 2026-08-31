#include "config/config.hpp"
#include "events.pb.h"
#include "instance.hpp"
#include "main.hpp"
#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include <pb_encode.h>
#include <stdint.h>
#include <utils.h>

GuitarHeroGuitarButtonMapping::GuitarHeroGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void GuitarHeroGuitarButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void GuitarHeroGuitarButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->tapOrange |= m_lastValue;
        break;
    }
}
void GuitarHeroGuitarButtonMapping::update_switch(uint8_t *buf)
{
    SwitchInputReport *report = (SwitchInputReport *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
    case GuitarHeroGuitar_TapRed:
    case GuitarHeroGuitar_TapYellow:
    case GuitarHeroGuitar_TapBlue:
    case GuitarHeroGuitar_TapOrange:
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps2(uint8_t *buf)
{
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->green |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->red |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->yellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->blue |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->orange |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        // TODO: get a ps2 wt guitar pcb and see if we can figure this one out
        // report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->tapOrange |= m_lastValue;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3GuitarHeroGuitar_Data_t *report = (PS3GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->tapOrange |= m_lastValue;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->soloGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->soloRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->soloYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->soloBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->soloOrange |= m_lastValue;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps5(uint8_t *buf)
{
    PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->soloGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->soloRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->soloYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->soloBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->soloOrange |= m_lastValue;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->tapOrange |= m_lastValue;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGuitarHeroGuitar_Data_t *report = (OGXboxGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->tapGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->tapRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->tapYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->tapBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->tapOrange |= m_lastValue;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarButtonMapping::update_xboxone(uint8_t *buf)
{
    XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case GuitarHeroGuitar_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroGuitar_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapGreen:
        report->soloGreen |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapRed:
        report->soloRed |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapYellow:
        report->soloYellow |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapBlue:
        report->soloBlue |= m_lastValue;
        break;
    case GuitarHeroGuitar_TapOrange:
        report->soloOrange |= m_lastValue;
        break;
    default:
        break;
    }
}

GuitarHeroGuitarGamepadButtonMapping::GuitarHeroGuitarGamepadButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadButtonMapping(mapping, std::move(input), id, profile)
{
}
void GuitarHeroGuitarGamepadButtonMapping::update_ps2(uint8_t *buf)
{
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->green |= m_lastValue;
        break;
    case Gamepad_B:
        report->red |= m_lastValue;
        break;
    case Gamepad_X:
        report->blue |= m_lastValue;
        break;
    case Gamepad_Y:
        report->yellow |= m_lastValue;
        break;
    case Gamepad_Start:
        report->start |= m_lastValue;
        break;
    case Gamepad_Back:
        report->back |= m_lastValue;
        break;
    case Gamepad_Capture:
        break;
    case Gamepad_LeftShoulder:
        report->leftShoulder |= m_lastValue;
        break;
    case Gamepad_RightShoulder:
        report->rightShoulder |= m_lastValue;
        break;
    case Gamepad_LeftThumbClick:
        report->leftThumbClick |= m_lastValue;
        break;
    case Gamepad_RightThumbClick:
        report->rightThumbClick |= m_lastValue;
        break;
    case Gamepad_DpadUp:
        report->psDpadUp |= m_lastValue;
        report->dpadUp |= m_lastValue;
        break;
    case Gamepad_DpadDown:
        report->psDpadDown |= m_lastValue;
        report->dpadDown |= m_lastValue;
        break;
    case Gamepad_DpadLeft:
        report->psDpadLeft |= m_lastValue;
        break;
    case Gamepad_DpadRight:
        report->psDpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}

const uint8_t GuitarHeroGuitarAxisMapping::gh5_slider_mapping[] = {
    0x80, 0x15, 0x4D, 0x30, 0x9A, 0x99, 0x66,
    0x65, 0xC9, 0xC7, 0xC8, 0xC6, 0xAF, 0xAD,
    0xAE, 0xAC, 0xFF, 0xFB, 0xFD, 0xF9, 0xFE,
    0xFA, 0xFC, 0xF8, 0xE6, 0xE2, 0xE4, 0xE0,
    0xE5, 0xE1, 0xE3, 0xDF};

GuitarHeroGuitarAxisMapping::GuitarHeroGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.ghAxis == GuitarHeroGuitar_Whammy), m_supports_slider(profile->supports_slider)
{
}

void GuitarHeroGuitarAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void GuitarHeroGuitarAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)buf;
    switch (m_mapping.mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitar_Whammy:
        report->whammy = m_calibratedValue >> 11;
        break;
    case GuitarHeroGuitar_Tilt:
        // report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarAxisMapping::update_switch(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarAxisMapping::update_ps2(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    // tilt is flipped
    int tilt = m_calibratedValue - 32768;
    // tilt centers at 512 but isnt actually full range
    tilt = 512 - (tilt >> 8);
    PS3GuitarHeroGuitar_Data_t *report = (PS3GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitar_Tilt:
        report->tilt = tilt;
        break;
    }
}

void GuitarHeroGuitarAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarAxisMapping::update_ps5(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitar_Whammy:
        report->whammy = m_calibratedValue - 32768;
        break;
    case GuitarHeroGuitar_Tilt:
        report->tilt = m_calibratedValue - 32768;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxGuitarHeroGuitar_Data_t *report = (OGXboxGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitar_Whammy:
        report->whammy = m_calibratedValue - 32768;
        break;
    case GuitarHeroGuitar_Tilt:
        report->tilt = m_calibratedValue - 32768;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarAxisMapping::update_xboxone(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

GuitarHeroDrumsAxisMapping::GuitarHeroDrumsAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void GuitarHeroDrumsAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void GuitarHeroDrumsAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    // TODO: this one is a bit fun because we can only send one velocity at a time.
    // TODO: deal with stick now that gamepad isnt here
    WiiDrumDataFormat3_t *report = (WiiDrumDataFormat3_t *)buf;
    switch (m_mapping.mapping.mapping.ghDrumAxis)
    {
    case GuitarHeroDrums_RedPad:
        report->b = true;
        break;
    case GuitarHeroDrums_YellowPad:
        report->y = true;
        break;
    case GuitarHeroDrums_BluePad:
        report->x = true;
        break;
    case GuitarHeroDrums_OrangePad:
        report->rightShoulder = true;
        break;
    case GuitarHeroDrums_GreenPad:
        report->a = true;
        break;
    case GuitarHeroDrums_KickPedal:
        report->leftShoulder = true;
        break;
    // case GuitarHeroDrums_LeftStickX:
    //     report->leftStickX = m_calibratedValue >> 10;
    //     break;
    // case GuitarHeroDrums_LeftStickY:
    //     report->leftStickY = m_calibratedValue >> 10;
    //     break;
    default:
        break;
    }
}
void GuitarHeroDrumsAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing on switch
}

void GuitarHeroDrumsAxisMapping::update_ps2(uint8_t *buf)
{
    // not a thing on ps2
}

void GuitarHeroDrumsAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS3GuitarHeroDrums_Data_t *report = (PS3GuitarHeroDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghDrumAxis)
    {
    case GuitarHeroDrums_RedPad:
        report->redVelocity = m_calibratedValue >> 8;
        report->b = true;
        break;
    case GuitarHeroDrums_YellowPad:
        report->yellowVelocity = m_calibratedValue >> 8;
        report->y = true;
        break;
    case GuitarHeroDrums_BluePad:
        report->blueVelocity = m_calibratedValue >> 8;
        report->x = true;
        break;
    case GuitarHeroDrums_OrangePad:
        report->orangeVelocity = m_calibratedValue >> 8;
        report->rightShoulder = true;
        break;
    case GuitarHeroDrums_GreenPad:
        report->greenVelocity = m_calibratedValue >> 8;
        report->a = true;
        break;
    case GuitarHeroDrums_KickPedal:
        report->kickVelocity = m_calibratedValue >> 8;
        report->leftShoulder = true;
        break;
    }
}

void GuitarHeroDrumsAxisMapping::update_ps4(uint8_t *buf)
{
    // not a thing on ps4
}

void GuitarHeroDrumsAxisMapping::update_ps5(uint8_t *buf)
{
    // not a thing on ps4
}

void GuitarHeroDrumsAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputGuitarHeroDrums_Data_t *report = (XInputGuitarHeroDrums_Data_t *)buf;

    switch (m_mapping.mapping.mapping.ghDrumAxis)
    {
    case GuitarHeroDrums_RedPad:
        report->redVelocity = m_calibratedValue - 32768;
        report->b = true;
        break;
    case GuitarHeroDrums_YellowPad:
        report->yellowVelocity = m_calibratedValue - 32768;
        report->y = true;
        break;
    case GuitarHeroDrums_BluePad:
        report->blueVelocity = m_calibratedValue - 32768;
        report->x = true;
        break;
    case GuitarHeroDrums_OrangePad:
        report->orangeVelocity = m_calibratedValue - 32768;
        report->rightShoulder = true;
        break;
    case GuitarHeroDrums_GreenPad:
        report->greenVelocity = m_calibratedValue - 32768;
        report->a = true;
        break;
    case GuitarHeroDrums_KickPedal:
        report->kickVelocity = m_calibratedValue - 32768;
        report->leftShoulder = true;
        break;
    }
}
void GuitarHeroDrumsAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxGuitarHeroDrums_Data_t *report = (OGXboxGuitarHeroDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghDrumAxis)
    {
    case GuitarHeroDrums_RedPad:
        report->redVelocity = m_calibratedValue - 32768;
        report->b = true;
        break;
    case GuitarHeroDrums_YellowPad:
        report->yellowVelocity = m_calibratedValue - 32768;
        report->y = true;
        break;
    case GuitarHeroDrums_BluePad:
        report->blueVelocity = m_calibratedValue - 32768;
        report->x = true;
        break;
    case GuitarHeroDrums_OrangePad:
        report->orangeVelocity = m_calibratedValue - 32768;
        report->rightShoulder = true;
        break;
    case GuitarHeroDrums_GreenPad:
        report->greenVelocity = m_calibratedValue - 32768;
        report->a = true;
        break;
    case GuitarHeroDrums_KickPedal:
        report->kickVelocity = m_calibratedValue - 32768;
        report->leftShoulder = true;
        break;
    }
}
void GuitarHeroDrumsAxisMapping::update_xboxone(uint8_t *buf)
{
}

GuitarHeroArcadeButtonMapping::GuitarHeroArcadeButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void GuitarHeroArcadeButtonMapping::update_hid(uint8_t *buf)
{
    ArcadeGuitarHeroGuitar_Data_t *report = (ArcadeGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghaButton)
    {
    case GuitarHeroArcade_Green:
        report->a |= m_lastValue;
        break;
    case GuitarHeroArcade_Red:
        report->b |= m_lastValue;
        break;
    case GuitarHeroArcade_Yellow:
        report->y |= m_lastValue;
        break;
    case GuitarHeroArcade_Blue:
        report->x |= m_lastValue;
        break;
    case GuitarHeroArcade_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroArcade_Side:
        report->side |= m_lastValue;
        break;
    }
}
void GuitarHeroArcadeButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
}
void GuitarHeroArcadeButtonMapping::update_switch(uint8_t *buf)
{
}

void GuitarHeroArcadeButtonMapping::update_ps2(uint8_t *buf)
{
}

void GuitarHeroArcadeButtonMapping::update_ps3(uint8_t *buf)
{
}

void GuitarHeroArcadeButtonMapping::update_ps4(uint8_t *buf)
{
}

void GuitarHeroArcadeButtonMapping::update_ps5(uint8_t *buf)
{
}

void GuitarHeroArcadeButtonMapping::update_xinput(uint8_t *buf)
{
}
void GuitarHeroArcadeButtonMapping::update_ogxbox(uint8_t *buf)
{
}
void GuitarHeroArcadeButtonMapping::update_xboxone(uint8_t *buf)
{
}

GuitarHeroArcadeAxisMapping::GuitarHeroArcadeAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void GuitarHeroArcadeAxisMapping::update_hid(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    ArcadeGuitarHeroGuitar_Data_t *report = (ArcadeGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghaAxis)
    {
    case GuitarHeroArcade_Tilt:
        report->tilt = m_calibratedValue - 32768;
        break;
    }
}
void GuitarHeroArcadeAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
}
void GuitarHeroArcadeAxisMapping::update_switch(uint8_t *buf)
{
}

void GuitarHeroArcadeAxisMapping::update_ps2(uint8_t *buf)
{
}

void GuitarHeroArcadeAxisMapping::update_ps3(uint8_t *buf)
{
}

void GuitarHeroArcadeAxisMapping::update_ps4(uint8_t *buf)
{
}

void GuitarHeroArcadeAxisMapping::update_ps5(uint8_t *buf)
{
}

void GuitarHeroArcadeAxisMapping::update_xinput(uint8_t *buf)
{
}
void GuitarHeroArcadeAxisMapping::update_ogxbox(uint8_t *buf)
{
}
void GuitarHeroArcadeAxisMapping::update_xboxone(uint8_t *buf)
{
}

LiveGuitarButtonMapping::LiveGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void LiveGuitarButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void LiveGuitarButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void LiveGuitarButtonMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void LiveGuitarButtonMapping::update_ps2(uint8_t *buf)
{
    // not a thing
}

void LiveGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3GHLGuitar_Data_t *report = (PS3GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitar_White1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = 0;
        break;
    case GuitarHeroLiveGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = 0xFF;
        break;
    default:
        break;
    }
}

void LiveGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    PS4GHLGuitar_Data_t *report = (PS4GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitar_White1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = 0;
        break;
    case GuitarHeroLiveGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = 0xFF;
        break;
    default:
        break;
    }
}

void LiveGuitarButtonMapping::update_ps5(uint8_t *buf)
{
    PS5GHLGuitar_Data_t *report = (PS5GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitar_White1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = 0;
        break;
    case GuitarHeroLiveGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = 0xFF;
        break;
    default:
        break;
    }
}

void LiveGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputGHLGuitar_Data_t *report = (XInputGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitar_White1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = INT16_MAX;
        break;
    case GuitarHeroLiveGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = INT16_MIN;
        break;
    default:
        break;
    }
}
void LiveGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGHLGuitar_Data_t *report = (OGXboxGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlButton)
    {
    case GuitarHeroLiveGuitar_White1:
        report->x |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White2:
        report->leftShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_White3:
        report->rightShoulder |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black1:
        report->a |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black2:
        report->b |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_Black3:
        report->y |= m_lastValue;
        break;
    case GuitarHeroLiveGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        report->strumBar = INT16_MAX;
        break;
    case GuitarHeroLiveGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        report->strumBar = INT16_MIN;
        break;
    default:
        break;
    }
}
void LiveGuitarButtonMapping::update_xboxone(uint8_t *buf)
{
}

LiveGuitarAxisMapping::LiveGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.ghlAxis == GuitarHeroLiveGuitar_Whammy)
{
}

void LiveGuitarAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void LiveGuitarAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void LiveGuitarAxisMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void LiveGuitarAxisMapping::update_ps2(uint8_t *buf)
{
    // not a thing
}

void LiveGuitarAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS3GHLGuitar_Data_t *report = (PS3GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroLiveGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    }
}

void LiveGuitarAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS4GHLGuitar_Data_t *report = (PS4GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroLiveGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void LiveGuitarAxisMapping::update_ps5(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS5GHLGuitar_Data_t *report = (PS5GHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitar_Whammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroLiveGuitar_Tilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void LiveGuitarAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputGHLGuitar_Data_t *report = (XInputGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitar_Whammy:
        report->whammy = m_calibratedValue - 32768;
        break;
    case GuitarHeroLiveGuitar_Tilt:
        report->tilt = m_calibratedValue - 32768;
        break;
    }
}
void LiveGuitarAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxGHLGuitar_Data_t *report = (OGXboxGHLGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghlAxis)
    {
    case GuitarHeroLiveGuitar_Whammy:
        report->whammy = m_calibratedValue - 32768;
        break;
    case GuitarHeroLiveGuitar_Tilt:
        report->tilt = m_calibratedValue - 32768;
        break;
    }
}
void LiveGuitarAxisMapping::update_xboxone(uint8_t *buf)
{
}

DJHTurntableButtonMapping::DJHTurntableButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void DJHTurntableButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void DJHTurntableButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // TODO: we have to deal with data formats probably
    WiiTurntableIntermediateFormat3_t *report = (WiiTurntableIntermediateFormat3_t *)buf;
    switch (m_mapping.mapping.mapping.djhButton)
    {
    case DJHTurntable_LeftGreen:
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntable_LeftRed:
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntable_LeftBlue:
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntable_RightGreen:
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntable_RightRed:
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntable_RightBlue:
        report->rightBlue |= m_lastValue;
        break;
    }
}
void DJHTurntableButtonMapping::update_switch(uint8_t *buf)
{
    // no switch turntable
}

void DJHTurntableButtonMapping::update_ps2(uint8_t *buf)
{
    // no ps2 turntable
}

void DJHTurntableButtonMapping::update_ps3(uint8_t *buf)
{
    PS3DJHTurntable_Data_t *report = (PS3DJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.mapping.djhButton)
    {
    case DJHTurntable_LeftGreen:
        report->a |= m_lastValue;
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntable_LeftRed:
        report->b |= m_lastValue;
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntable_LeftBlue:
        report->x |= m_lastValue;
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntable_RightGreen:
        report->a |= m_lastValue;
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntable_RightRed:
        report->b |= m_lastValue;
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntable_RightBlue:
        report->x |= m_lastValue;
        report->rightBlue |= m_lastValue;
        break;
    default:
        break;
    }
}

void DJHTurntableButtonMapping::update_ps4(uint8_t *buf)
{
    // no ps4 turntable
}

void DJHTurntableButtonMapping::update_ps5(uint8_t *buf)
{
    // no ps5 turntable
}

void DJHTurntableButtonMapping::update_xinput(uint8_t *buf)
{
    XInputDJHTurntable_Data_t *report = (XInputDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.mapping.djhButton)
    {
    case DJHTurntable_LeftGreen:
        report->a |= m_lastValue;
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntable_LeftRed:
        report->b |= m_lastValue;
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntable_LeftBlue:
        report->x |= m_lastValue;
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntable_RightGreen:
        report->a |= m_lastValue;
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntable_RightRed:
        report->b |= m_lastValue;
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntable_RightBlue:
        report->x |= m_lastValue;
        report->rightBlue |= m_lastValue;
        break;
    default:
        break;
    }
}
void DJHTurntableButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxDJHTurntable_Data_t *report = (OGXboxDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.mapping.djhButton)
    {
    case DJHTurntable_LeftGreen:
        report->a |= m_lastValue;
        report->leftGreen |= m_lastValue;
        break;
    case DJHTurntable_LeftRed:
        report->b |= m_lastValue;
        report->leftRed |= m_lastValue;
        break;
    case DJHTurntable_LeftBlue:
        report->x |= m_lastValue;
        report->leftBlue |= m_lastValue;
        break;
    case DJHTurntable_RightGreen:
        report->a |= m_lastValue;
        report->rightGreen |= m_lastValue;
        break;
    case DJHTurntable_RightRed:
        report->b |= m_lastValue;
        report->rightRed |= m_lastValue;
        break;
    case DJHTurntable_RightBlue:
        report->x |= m_lastValue;
        report->rightBlue |= m_lastValue;
        break;
    default:
        break;
    }
}
void DJHTurntableButtonMapping::update_xboxone(uint8_t *buf)
{
    // not a thing
}

DJHTurntableAxisMapping::DJHTurntableAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}

void DJHTurntableAxisMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void DJHTurntableAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    WiiTurntableIntermediateFormat3_t intermediate;
    WiiTurntableDataFormat3_t *report = (WiiTurntableDataFormat3_t *)buf;
    switch (m_mapping.mapping.mapping.djhAxis)
    {
    case DJHTurntable_LeftVelocity:
        intermediate.leftTableVelocity = m_calibratedValue >> 10;
        report->leftTableVelocity40 = intermediate.leftTableVelocity40;
        report->leftTableVelocity5 = intermediate.leftTableVelocity5;
        break;
    case DJHTurntable_RightVelocity:
        intermediate.rightTableVelocity = m_calibratedValue >> 10;
        report->rightTableVelocity0 = intermediate.rightTableVelocity0;
        report->rightTableVelocity21 = intermediate.rightTableVelocity21;
        report->rightTableVelocity43 = intermediate.rightTableVelocity43;
        report->rightTableVelocity5 = intermediate.rightTableVelocity5;
        break;
    case DJHTurntable_EffectsKnob:
        intermediate.effectsKnob = m_calibratedValue >> 11;
        report->effectsKnob20 = intermediate.effectsKnob20;
        report->effectsKnob43 = intermediate.effectsKnob43;
        break;
    case DJHTurntable_Crossfader:
        report->crossfader = m_calibratedValue >> 12;
        break;
    default:
        break;
    }
}
void DJHTurntableAxisMapping::update_switch(uint8_t *buf)
{
    // no switch turntable
}

void DJHTurntableAxisMapping::update_ps2(uint8_t *buf)
{
    // no ps2 turntable
}

void DJHTurntableAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS3DJHTurntable_Data_t *report = (PS3DJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.mapping.djhAxis)
    {
    case DJHTurntable_LeftVelocity:
        report->leftTableVelocity = m_calibratedValue >> 8;
        break;
    case DJHTurntable_RightVelocity:
        report->rightTableVelocity = m_calibratedValue >> 8;
        break;
    case DJHTurntable_EffectsKnob:
        report->effectsKnob = m_calibratedValue >> 6;
        break;
    case DJHTurntable_Crossfader:
        report->crossfader = m_calibratedValue >> 6;
        break;
    }
}

void DJHTurntableAxisMapping::update_ps4(uint8_t *buf)
{
    // no ps4 turntable
}

void DJHTurntableAxisMapping::update_ps5(uint8_t *buf)
{
    // no ps5 turntable
}

void DJHTurntableAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XInputDJHTurntable_Data_t *report = (XInputDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.mapping.djhAxis)
    {
    case DJHTurntable_LeftVelocity:
        report->leftTableVelocity = m_calibratedValue - 32768;
        break;
    case DJHTurntable_RightVelocity:
        report->rightTableVelocity = m_calibratedValue - 32768;
        break;
    case DJHTurntable_EffectsKnob:
        report->effectsKnob = m_calibratedValue - 32768;
        break;
    case DJHTurntable_Crossfader:
        report->crossfader = m_calibratedValue - 32768;
        break;
    default:
        break;
    }
}
void DJHTurntableAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    OGXboxDJHTurntable_Data_t *report = (OGXboxDJHTurntable_Data_t *)buf;
    switch (m_mapping.mapping.mapping.djhAxis)
    {
    case DJHTurntable_LeftVelocity:
        report->leftTableVelocity = m_calibratedValue - 32768;
        break;
    case DJHTurntable_RightVelocity:
        report->rightTableVelocity = m_calibratedValue - 32768;
        break;
    case DJHTurntable_EffectsKnob:
        report->effectsKnob = m_calibratedValue - 32768;
        break;
    case DJHTurntable_Crossfader:
        report->crossfader = m_calibratedValue - 32768;
        break;
    default:
        break;
    }
}
void DJHTurntableAxisMapping::update_xboxone(uint8_t *buf)
{
    // not a thing
}
