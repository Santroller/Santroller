#include "devices/usb/host/xinput_tick_helpers.h"
#include "protocols/pro_keys.hpp"
#include "devices/usb/host/gh_slider_helpers.h"

uint16_t xinput_rb_drum_velocity(int16_t velocity)
{
    uint16_t distance = velocity < 0 ? (uint16_t)~velocity : (uint16_t)velocity;
    return UINT16_MAX - (distance << 1);
}

XInputRockBandDrumHits xinput_decode_rb_drums(const XInputRockBandDrums_Data_t *data, bool rb2)
{
    XInputRockBandDrumHits hits = {};
    if (!rb2)
    {
        hits.red_pad = data->b ? UINT16_MAX : 0;
        hits.yellow_pad = data->y ? UINT16_MAX : 0;
        hits.blue_pad = data->x ? UINT16_MAX : 0;
        hits.green_pad = data->a ? UINT16_MAX : 0;
        return hits;
    }
    bool green = data->a;
    bool red = data->b;
    bool yellow = data->y;
    bool blue = data->x;
    bool pad = data->padFlag;
    bool cymbal = data->cymbalFlag;
    bool up = data->dpadUp;
    bool down = data->dpadDown;
    uint16_t red_velocity = xinput_rb_drum_velocity(data->redVelocity);
    uint16_t yellow_velocity = xinput_rb_drum_velocity(data->yellowVelocity);
    uint16_t blue_velocity = xinput_rb_drum_velocity(data->blueVelocity);
    uint16_t green_velocity = xinput_rb_drum_velocity(data->greenVelocity);
    if (pad && cymbal)
    {
        // Only the d-pad is checked to resolve a pad+cymbal hit, as the colour flag can be
        // released before the d-pad is (particularly at high polling rates)
        if (!red && data->redVelocity)
        {
            // Pad and cymbal of the same colour hit together: the cymbal velocity goes in
            // redVelocity, without red being pressed
            if (up)
            {
                hits.yellow_cymbal = red_velocity;
                hits.yellow_pad = yellow_velocity;
            }
            else if (down)
            {
                hits.blue_cymbal = red_velocity;
                hits.blue_pad = blue_velocity;
            }
            else
            {
                hits.green_cymbal = red_velocity;
                hits.green_pad = green_velocity;
            }
            return hits;
        }
        if (up)
        {
            hits.yellow_cymbal = yellow_velocity;
            yellow = false;
        }
        if (down)
        {
            hits.blue_cymbal = blue_velocity;
            blue = false;
        }
        if (!(up || down))
        {
            hits.green_cymbal = green_velocity;
            green = false;
        }
        cymbal = false;
    }
    if (pad)
    {
        if (red)
            hits.red_pad = red_velocity;
        if (yellow)
            hits.yellow_pad = yellow_velocity;
        if (blue)
            hits.blue_pad = blue_velocity;
        if (green)
            hits.green_pad = green_velocity;
    }
    if (cymbal)
    {
        if (yellow)
            hits.yellow_cymbal = yellow_velocity;
        if (blue)
            hits.blue_cymbal = blue_velocity;
        if (green)
            hits.green_cymbal = green_velocity;
    }
    return hits;
}

uint16_t xinput_rb_drum_axis(const uint8_t *ep_in_buf, RockBandDrumsAxisType axis, bool rb2)
{
    auto hits = xinput_decode_rb_drums((const XInputRockBandDrums_Data_t *)ep_in_buf, rb2);
    switch (axis)
    {
    case RockBandDrums_RedPad:
        return hits.red_pad;
    case RockBandDrums_YellowPad:
        return hits.yellow_pad;
    case RockBandDrums_BluePad:
        return hits.blue_pad;
    case RockBandDrums_GreenPad:
        return hits.green_pad;
    case RockBandDrums_YellowCymbal:
        return hits.yellow_cymbal;
    case RockBandDrums_BlueCymbal:
        return hits.blue_cymbal;
    case RockBandDrums_GreenCymbal:
        return hits.green_cymbal;
    default:
        return 0;
    }
}

uint16_t xinput_gh_drum_axis(const uint8_t *ep_in_buf, GuitarHeroDrumsAxisType axis)
{
    auto data = (const XInputGuitarHeroDrums_Data_t *)ep_in_buf;
    switch (axis)
    {
    case GuitarHeroDrums_RedPad:
        return data->redVelocity << 9;
    case GuitarHeroDrums_YellowPad:
        return data->yellowVelocity << 9;
    case GuitarHeroDrums_BluePad:
        return data->blueVelocity << 9;
    case GuitarHeroDrums_OrangePad:
        return data->orangeVelocity << 9;
    case GuitarHeroDrums_GreenPad:
        return data->greenVelocity << 9;
    case GuitarHeroDrums_KickPedal:
        return data->kickVelocity << 9;
    default:
        return 0;
    }
}

bool xinput_tick_digital_impl(const uint8_t* ep_in_buf, SubType subtype, proto_Output &type, bool is_wt, bool rb2)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        auto data = (XInputGamepad_Data_t *)ep_in_buf;
        // RB2 kits press the face buttons and d-pad when hitting pads and cymbals, only
        // treat them as navigation when no hit is flagged
        if (subtype == RockBandDrums)
        {
            auto drums = (const XInputRockBandDrums_Data_t *)ep_in_buf;
            if (drums->padFlag || drums->cymbalFlag)
            {
                switch (type.mapping.gamepadButton)
                {
                case Gamepad_A:
                case Gamepad_B:
                case Gamepad_X:
                case Gamepad_Y:
                case Gamepad_DpadUp:
                case Gamepad_DpadDown:
                case Gamepad_DpadLeft:
                case Gamepad_DpadRight:
                    return false;
                default:
                    break;
                }
            }
        }
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
    case ProGuitarMustang:
    case ProGuitarSquire:
        if (type.which_mapping == proto_Output_proButton_tag)
        {
            const auto *data = reinterpret_cast<const XInputRockBandProGuitar_Data_t *>(ep_in_buf);
            switch (type.mapping.proButton)
            {
            case ProGuitar_Green:
                return data->green;
            case ProGuitar_Red:
                return data->red;
            case ProGuitar_Yellow:
                return data->yellow;
            case ProGuitar_Blue:
                return data->blue;
            case ProGuitar_Orange:
                return data->orange;
            case ProGuitar_SoloGreen:
                return data->green && data->soloFlag;
            case ProGuitar_SoloRed:
                return data->red && data->soloFlag;
            case ProGuitar_SoloYellow:
                return data->yellow && data->soloFlag;
            case ProGuitar_SoloBlue:
                return data->blue && data->soloFlag;
            case ProGuitar_SoloOrange:
                return data->orange && data->soloFlag;
            case ProGuitar_Pedal:
                return data->pedal;
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
        if (type.which_mapping == proto_Output_proKeyboardButton_tag)
        {
            const auto *data = reinterpret_cast<const XInputRockBandKeyboard_Data_t *>(ep_in_buf);
            switch (type.mapping.proKeyboardButton)
            {
            case ProKeyboardOverdrive:
                return data->overdrive;
            default:
                return false;
            }
        }
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
    case DjHeroTurntable:
        if (type.which_mapping == proto_Output_djhButton_tag)
        {
            auto data = (XInputDJHTurntable_Data_t *)ep_in_buf;
            switch (type.mapping.djhButton)
            {
            case DJHTurntable_LeftGreen:
                return data->leftGreen;
            case DJHTurntable_LeftRed:
                return data->leftRed;
            case DJHTurntable_LeftBlue:
                return data->leftBlue;
            case DJHTurntable_RightGreen:
                return data->rightGreen;
            case DJHTurntable_RightRed:
                return data->rightRed;
            case DJHTurntable_RightBlue:
                return data->rightBlue;
            default:
                return false;
            }
        }
        return false;
    case RockBandDrums:
        if (type.which_mapping == proto_Output_rbDrumButton_tag)
        {
            auto data = (const XInputRockBandDrums_Data_t *)ep_in_buf;
            switch (type.mapping.rbDrumButton)
            {
            case RockBandDrums_Kick1Pedal:
                return data->kick1;
            case RockBandDrums_Kick2Pedal:
                return data->kick2;
            default:
                return false;
            }
        }
        if (type.which_mapping == proto_Output_rbDrumAxis_tag)
            return xinput_rb_drum_axis(ep_in_buf, type.mapping.rbDrumAxis, rb2) != 0;
        return false;
    case GuitarHeroDrums:
        if (type.which_mapping == proto_Output_ghDrumAxis_tag)
            return xinput_gh_drum_axis(ep_in_buf, type.mapping.ghDrumAxis) != 0;
        return false;
    default:
        return false;
    }

    return false;
}

uint16_t xinput_turntable_velocity(int16_t velocity)
{
    int32_t value = (int32_t)velocity * XINPUT_TURNTABLE_VELOCITY_SCALE + 32768;
    return value < 0 ? 0 : value > UINT16_MAX ? UINT16_MAX : value;
}

uint16_t xinput_tick_analog_impl(const uint8_t* ep_in_buf, SubType subtype, proto_Output &type, bool rb2)
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
    case DjHeroTurntable:
        if (type.which_mapping == proto_Output_djhAxis_tag)
        {
            auto data = (XInputDJHTurntable_Data_t *)ep_in_buf;
            switch (type.mapping.djhAxis)
            {
            case DJHTurntable_LeftVelocity:
                return xinput_turntable_velocity(data->leftTableVelocity);
            case DJHTurntable_RightVelocity:
                return xinput_turntable_velocity(data->rightTableVelocity);
            case DJHTurntable_EffectsKnob:
                return (uint16_t)data->effectsKnob ^ 0x8000;
            case DJHTurntable_Crossfader:
                return (uint16_t)data->crossfader ^ 0x8000;
            default:
                return 0;
            }
        }
        break;
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
                return data->tilt << 9;
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
                return (0x7f - data->pedalAnalog) << 9;
            case ProKeyboardTouchPad:
                return data->touchPad << 9;
            default:
                return 0;
            }
        }
        break;
    case RockBandDrums:
        if (type.which_mapping == proto_Output_rbDrumAxis_tag)
            return xinput_rb_drum_axis(ep_in_buf, type.mapping.rbDrumAxis, rb2);
        break;
    case GuitarHeroDrums:
        if (type.which_mapping == proto_Output_ghDrumAxis_tag)
            return xinput_gh_drum_axis(ep_in_buf, type.mapping.ghDrumAxis);
        break;
    default:
        break;
    }

    return 0;
}

uint16_t xinput_tick_button_pressure_impl(const uint8_t *ep_in_buf, SubType subtype,
                                          proto_Output &type, bool is_wt, bool rb2)
{
    if (!xinput_tick_digital_impl(ep_in_buf, subtype, type, is_wt, rb2))
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
    if (type.which_mapping == proto_Output_rbDrumAxis_tag ||
        type.which_mapping == proto_Output_ghDrumAxis_tag)
        return xinput_tick_analog_impl(ep_in_buf, subtype, type, rb2);

    return UINT16_MAX;
}
