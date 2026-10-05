#pragma once

#include "protocols/xinput.hpp"
#include "protocols/pro_keys.hpp"
#include "config.pb.h"
#include "devices/usb/host/gh_slider_helpers.h"

// Shared tick_digital and tick_analog implementations for XInput devices
// These functions take the buffer and subtype as parameters

inline bool xinput_tick_digital_impl(const uint8_t* ep_in_buf, SubType subtype, proto_Output &type, bool is_wt = false)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        auto data = (XInputGamepad_Data_t *)ep_in_buf;
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:
            return data->a;
        case Gamepad_B:
            return data->b;
        case Gamepad_X:
            return data->x;
        case Gamepad_Y:
            return data->y;
        case Gamepad_LeftShoulder:
            return data->leftShoulder;
        case Gamepad_RightShoulder:
            return data->rightShoulder;
        case Gamepad_Back:
            return data->back;
        case Gamepad_Start:
            return data->start;
        case Gamepad_LeftThumbClick:
            return data->leftThumbClick;
        case Gamepad_RightThumbClick:
            return data->rightThumbClick;
        case Gamepad_Guide:
            return data->guide;
        case Gamepad_DpadUp:
            return data->dpadUp;
        case Gamepad_DpadDown:
            return data->dpadDown;
        case Gamepad_DpadLeft:
            return data->dpadLeft;
        case Gamepad_DpadRight:
            return data->dpadRight;
        default:
            return false;
        }
    }
    switch (subtype)
    {
    case GuitarHeroGuitar:
        if (type.which_mapping == proto_Output_ghButton_tag)
        {
            auto data = (XInputGuitarHeroGuitar_Data_t *)ep_in_buf;
            uint8_t slider = (((uint16_t)data->slider + 0x80) >> 8) ^ 0x80;
            uint8_t frets = is_wt ? decode_ghwt_slider(slider) : decode_gh5_slider(slider);
            switch (type.mapping.ghButton)
            {
            case GuitarHeroGuitar_Green:
                return data->a;
            case GuitarHeroGuitar_Red:
                return data->b;
            case GuitarHeroGuitar_Yellow:
                return data->y;
            case GuitarHeroGuitar_Blue:
                return data->x;
            case GuitarHeroGuitar_Orange:
                return data->leftShoulder;
            case GuitarHeroGuitar_Pedal:
                return data->rightShoulder;
            case GuitarHeroGuitar_TapGreen:
                return frets & 0b00001;
            case GuitarHeroGuitar_TapRed:
                return frets & 0b00010;
            case GuitarHeroGuitar_TapYellow:
                return frets & 0b00100;
            case GuitarHeroGuitar_TapBlue:
                return frets & 0b01000;
            case GuitarHeroGuitar_TapOrange:
                return frets & 0b10000;
            default:
                return false;
            }
        }
        return false;
    case RockBandGuitar:
        if (type.which_mapping == proto_Output_rbButton_tag)
        {
            auto data = (XInputRockBandGuitar_Data_t *)ep_in_buf;
            switch (type.mapping.rbButton)
            {
            case RockBandGuitar_Green:
                return data->a && !data->solo;
            case RockBandGuitar_Red:
                return data->b && !data->solo;
            case RockBandGuitar_Yellow:
                return data->y && !data->solo;
            case RockBandGuitar_Blue:
                return data->x && !data->solo;
            case RockBandGuitar_Orange:
                return data->leftShoulder && !data->solo;
            case RockBandGuitar_SoloGreen:
                return data->a && data->solo;
            case RockBandGuitar_SoloRed:
                return data->b && data->solo;
            case RockBandGuitar_SoloYellow:
                return data->y && data->solo;
            case RockBandGuitar_SoloBlue:
                return data->x && data->solo;
            case RockBandGuitar_SoloOrange:
                return data->leftShoulder && data->solo;
            default:
                return false;
            }
        }
        return false;
    case LiveGuitar:
        if (type.which_mapping == proto_Output_ghlButton_tag)
        {
            auto data = (XInputGHLGuitar_Data_t *)ep_in_buf;
            switch (type.mapping.ghlButton)
            {
            case GuitarHeroLiveGuitar_Black1:
                return data->a;
            case GuitarHeroLiveGuitar_Black2:
                return data->b;
            case GuitarHeroLiveGuitar_Black3:
                return data->y;
            case GuitarHeroLiveGuitar_White1:
                return data->x;
            case GuitarHeroLiveGuitar_White2:
                return data->leftShoulder;
            case GuitarHeroLiveGuitar_White3:
                return data->rightShoulder;
            case GuitarHeroLiveGuitar_StrumUp:
                return data->strumBar > 0;
            case GuitarHeroLiveGuitar_StrumDown:
                return data->strumBar < 0;
            default:
                return false;
            }
        }
        return false;
    case ProKeys:
        if (type.which_mapping == proto_Output_proKeySingle_tag ||
            type.which_mapping == proto_Output_proKeyMultiple_tag)
        {
            auto data = (XInputRockBandKeyboard_Data_t *)ep_in_buf;
            if (type.which_mapping == proto_Output_proKeySingle_tag)
                return pro_keyboard_key_pressed(data->key1, data->key2, data->key3,
                                                data->velocities, type.mapping.proKeySingle);
            return pro_keyboard_any_key_pressed(data->key1, data->key2, data->key3,
                                                data->velocities, type.mapping.proKeyMultiple);
        }
        return false;
    default:
        return false;
    }

    return false;
}

inline uint16_t xinput_tick_analog_impl(const uint8_t* ep_in_buf, SubType subtype, proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        auto data = (XInputGamepad_Data_t *)ep_in_buf;
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_LeftTrigger:
            return data->leftTrigger << 8;
        case Gamepad_RightTrigger:
            return data->rightTrigger << 8;
        case Gamepad_LeftStickX:
            return (uint16_t)data->leftStickX ^ 0x8000;
        case Gamepad_LeftStickY:
            return (uint16_t)data->leftStickY ^ 0x8000;
        case Gamepad_RightStickX:
            return (uint16_t)data->rightStickX ^ 0x8000;
        case Gamepad_RightStickY:
            return (uint16_t)data->rightStickY ^ 0x8000;
        default:
            return 0;
        }

    }
    switch (subtype)
    {
    case GuitarHeroGuitar:
        if (type.which_mapping == proto_Output_ghAxis_tag)
        {
            auto data = (XInputGuitarHeroGuitar_Data_t *)ep_in_buf;
            switch (type.mapping.ghAxis)
            {
            case GuitarHeroGuitar_Whammy:
                return (uint16_t)data->whammy ^ 0x8000;
            case GuitarHeroGuitar_Tilt:
                return (uint16_t)data->tilt ^ 0x8000;
            default:
                return 0;
            }

        }
        break;
    case LiveGuitar:
        if (type.which_mapping == proto_Output_ghlAxis_tag)
        {
            auto data = (XInputGHLGuitar_Data_t *)ep_in_buf;
            switch (type.mapping.ghlAxis)
            {
            case GuitarHeroLiveGuitar_Whammy:
                return (uint16_t)data->whammy ^ 0x8000;
            case GuitarHeroLiveGuitar_Tilt:
                return (uint16_t)data->tilt ^ 0x8000;
            default:
                return 0;
            }

        }
        break;
    case RockBandGuitar:
        if (type.which_mapping == proto_Output_rbAxis_tag)
        {
            auto data = (XInputRockBandGuitar_Data_t *)ep_in_buf;
            switch (type.mapping.rbAxis)
            {
            case RockBandGuitar_Whammy:
                return (uint16_t)data->whammy ^ 0x8000;
            case RockBandGuitar_Tilt:
                return (uint16_t)data->tilt ^ 0x8000;
            case RockBandGuitar_Pickup:
                return data->pickup << 8;
            default:
                return 0;
            }
        }
        break;
    case ProGuitarMustang:
    case ProGuitarSquire:
        if (type.which_mapping == proto_Output_proAxis_tag)
        {
            auto data = (XInputRockBandProGuitar_Data_t *)ep_in_buf;
            switch (type.mapping.proAxis)
            {
            case ProGuitar_LowEFret:
                return data->lowEFret << 11;
            case ProGuitar_AFret:
                return data->aFret << 11;
            case ProGuitar_DFret:
                return data->dFret << 11;
            case ProGuitar_GFret:
                return data->gFret << 11;
            case ProGuitar_BFret:
                return data->bFret << 11;
            case ProGuitar_HighEFret:
                return data->highEFret << 11;
            case ProGuitar_LowEFretVelocity:
                return data->lowEFretVelocity << 9;
            case ProGuitar_AFretVelocity:
                return data->aFretVelocity << 9;
            case ProGuitar_DFretVelocity:
                return data->dFretVelocity << 9;
            case ProGuitar_GFretVelocity:
                return data->gFretVelocity << 9;
            case ProGuitar_BFretVelocity:
                return data->bFretVelocity << 9;
            case ProGuitar_HighEFretVelocity:
                return data->highEFretVelocity << 9;
            case ProGuitar_Tilt:
                return data->tilt << 8;
            case ProGuitar_AutoCalibrationMicrophone:
                return data->autoCal_Microphone << 8;
            case ProGuitar_AutoCalibrationLight:
                return data->autoCal_Light << 8;
            default:
                return 0;
            }
        }
        break;
    case ProKeys:
        if (type.which_mapping == proto_Output_proKeyboardAxis_tag)
        {
            auto data = (XInputRockBandKeyboard_Data_t *)ep_in_buf;
            switch (type.mapping.proKeyboardAxis)
            {
            case ProKeyboardPedal:
                return data->pedalAnalog << 9;
            case ProKeyboardTouchPad:
                return data->touchPad << 9;
            default:
                return 0;
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

inline uint16_t xinput_tick_button_pressure_impl(const uint8_t *ep_in_buf, SubType subtype,
                                                 proto_Output &type, bool is_wt = false)
{
    if (!xinput_tick_digital_impl(ep_in_buf, subtype, type, is_wt))
        return 0;

    if (subtype == ProKeys &&
        (type.which_mapping == proto_Output_proKeySingle_tag ||
         type.which_mapping == proto_Output_proKeyMultiple_tag))
    {
        const auto *data = reinterpret_cast<const XInputRockBandKeyboard_Data_t *>(ep_in_buf);
        if (type.which_mapping == proto_Output_proKeySingle_tag)
            return pro_keyboard_key_pressure(data->key1, data->key2, data->key3,
                                             data->velocities, type.mapping.proKeySingle);
        return pro_keyboard_key_range_pressure(data->key1, data->key2, data->key3,
                                               data->velocities, type.mapping.proKeyMultiple);
    }

    return UINT16_MAX;
}
