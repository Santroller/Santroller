#include "protocols/santroller_v1.hpp"
#include "devices/usb/host/xinput_tick_helpers.h"
#include "protocols/hid.hpp"
#include "protocols/xinput.hpp"

// Decoding for Santroller 1 HID reports (report id 1, layout depends on the subtype),
// shared by the BLE and USB host implementations.

bool santroller_v1_tick_digital(const uint8_t *report, SubType subtype, proto_Output &type)
{
    uint8_t hat = report[3] & 0x0F;
    bool up    = (hat == 0 || hat == 1 || hat == 7);
    bool down  = (hat == 3 || hat == 4 || hat == 5);
    bool left  = (hat == 5 || hat == 6 || hat == 7);
    bool right = (hat == 1 || hat == 2 || hat == 3);

    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (subtype)
        {
        case Dancepad:
        {
            auto data = (const Santroller1Gamepad_Data_t *)report;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_DpadUp:    return (data->dpad & 1) != 0;
            case Gamepad_DpadDown:  return ((data->dpad >> 1) & 1) != 0;
            case Gamepad_DpadLeft:  return ((data->dpad >> 2) & 1) != 0;
            case Gamepad_DpadRight: return ((data->dpad >> 3) & 1) != 0;
            case Gamepad_Back:      return data->back;
            case Gamepad_Start:     return data->start;
            default:                return false;
            }
        }
        case GuitarHeroGuitar:
        {
            auto data = (const Santroller1GuitarHeroGuitar_Data_t *)report;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:             return data->a;
            case Gamepad_B:             return data->b;
            case Gamepad_X:             return data->x;
            case Gamepad_Y:             return data->y;
            case Gamepad_LeftShoulder:  return data->leftShoulder;
            case Gamepad_RightShoulder: return data->rightShoulder;
            case Gamepad_Back:          return data->back;
            case Gamepad_Start:         return data->start;
            case Gamepad_Guide:         return data->guide;
            case Gamepad_DpadUp:        return up;
            case Gamepad_DpadDown:      return down;
            case Gamepad_DpadLeft:      return left;
            case Gamepad_DpadRight:     return right;
            default:                    return false;
            }
        }
        case RockBandGuitar:
        {
            auto data = (const Santroller1RockBandGuitar_Data_t *)report;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:             return data->a || data->soloGreen;
            case Gamepad_B:             return data->b || data->soloRed;
            case Gamepad_X:             return data->x || data->soloBlue;
            case Gamepad_Y:             return data->y || data->soloYellow;
            case Gamepad_LeftShoulder:  return data->leftShoulder || data->soloOrange;
            case Gamepad_Back:          return data->back;
            case Gamepad_Start:         return data->start;
            case Gamepad_Guide:         return data->guide;
            case Gamepad_DpadUp:        return up;
            case Gamepad_DpadDown:      return down;
            case Gamepad_DpadLeft:      return left;
            case Gamepad_DpadRight:     return right;
            default:                    return false;
            }
        }
        case LiveGuitar:
        {
            auto data = (const Santroller1GHLGuitar_Data_t *)report;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:              return data->a;
            case Gamepad_B:              return data->b;
            case Gamepad_X:              return data->x;
            case Gamepad_Y:              return data->y;
            case Gamepad_LeftShoulder:   return data->leftShoulder;
            case Gamepad_RightShoulder:  return data->rightShoulder;
            case Gamepad_Back:           return data->back;
            case Gamepad_Start:          return data->start;
            case Gamepad_LeftThumbClick: return data->leftThumbClick;
            case Gamepad_Guide:          return data->guide;
            case Gamepad_DpadUp:         return up;
            case Gamepad_DpadDown:       return down;
            case Gamepad_DpadLeft:       return left;
            case Gamepad_DpadRight:      return right;
            default:                     return false;
            }
        }
        case GuitarHeroDrums:
        {
            auto data = (const Santroller1GuitarHeroDrums_Data_t *)report;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:             return data->a;
            case Gamepad_B:             return data->b;
            case Gamepad_X:             return data->x;
            case Gamepad_Y:             return data->y;
            case Gamepad_LeftShoulder:  return data->leftShoulder;
            case Gamepad_RightShoulder: return data->rightShoulder;
            case Gamepad_Back:          return data->back;
            case Gamepad_Start:         return data->start;
            case Gamepad_Guide:         return data->guide;
            case Gamepad_DpadUp:        return up;
            case Gamepad_DpadDown:      return down;
            case Gamepad_DpadLeft:      return left;
            case Gamepad_DpadRight:     return right;
            default:                    return false;
            }
        }
        case RockBandDrums:
        {
            auto data = (const Santroller1RockBandDrums_Data_t *)report;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:             return data->a;
            case Gamepad_B:             return data->b;
            case Gamepad_X:             return data->x;
            case Gamepad_Y:             return data->y;
            case Gamepad_LeftShoulder:  return data->leftShoulder;
            case Gamepad_RightShoulder: return data->rightShoulder;
            case Gamepad_Back:          return data->back;
            case Gamepad_Start:         return data->start;
            case Gamepad_Guide:         return data->guide;
            case Gamepad_DpadUp:        return up;
            case Gamepad_DpadDown:      return down;
            case Gamepad_DpadLeft:      return left;
            case Gamepad_DpadRight:     return right;
            default:                    return false;
            }
        }
        case DjHeroTurntable:
        {
            auto data = (const Santroller1Turntable_Data_t *)report;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:         return data->a;
            case Gamepad_B:         return data->b;
            case Gamepad_X:         return data->x;
            case Gamepad_Y:         return data->y;
            case Gamepad_Back:      return data->back;
            case Gamepad_Start:     return data->start;
            case Gamepad_Guide:     return data->guide;
            case Gamepad_DpadUp:    return up;
            case Gamepad_DpadDown:  return down;
            case Gamepad_DpadLeft:  return left;
            case Gamepad_DpadRight: return right;
            default:                return false;
            }
        }
        default: // Gamepad and others
        {
            auto data = (const Santroller1Gamepad_Data_t *)report;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:               return data->a;
            case Gamepad_B:               return data->b;
            case Gamepad_X:               return data->x;
            case Gamepad_Y:               return data->y;
            case Gamepad_LeftShoulder:    return data->leftShoulder;
            case Gamepad_RightShoulder:   return data->rightShoulder;
            case Gamepad_Back:            return data->back;
            case Gamepad_Start:           return data->start;
            case Gamepad_LeftThumbClick:  return data->leftThumbClick;
            case Gamepad_RightThumbClick: return data->rightThumbClick;
            case Gamepad_Guide:           return data->guide;
            case Gamepad_Capture:         return data->capture;
            case Gamepad_DpadUp:          return up;
            case Gamepad_DpadDown:        return down;
            case Gamepad_DpadLeft:        return left;
            case Gamepad_DpadRight:       return right;
            default:                      return false;
            }
        }
        }
    }

    if (type.which_mapping == proto_Output_ghButton_tag && subtype == GuitarHeroGuitar)
    {
        auto data = (const Santroller1GuitarHeroGuitar_Data_t *)report;
        switch (type.mapping.ghButton)
        {
        case GuitarHeroGuitar_Green:  return data->a;
        case GuitarHeroGuitar_Red:    return data->b;
        case GuitarHeroGuitar_Yellow: return data->y;
        case GuitarHeroGuitar_Blue:   return data->x;
        case GuitarHeroGuitar_Orange: return data->leftShoulder;
        case GuitarHeroGuitar_Pedal:  return data->rightShoulder;
        default:                      return false;
        }
    }

    if (type.which_mapping == proto_Output_rbButton_tag && subtype == RockBandGuitar)
    {
        auto data = (const Santroller1RockBandGuitar_Data_t *)report;
        switch (type.mapping.rbButton)
        {
        case RockBandGuitar_Green:      return data->a;
        case RockBandGuitar_Red:        return data->b;
        case RockBandGuitar_Yellow:     return data->y;
        case RockBandGuitar_Blue:       return data->x;
        case RockBandGuitar_Orange:     return data->leftShoulder;
        case RockBandGuitar_SoloGreen:  return data->soloGreen;
        case RockBandGuitar_SoloRed:    return data->soloRed;
        case RockBandGuitar_SoloYellow: return data->soloYellow;
        case RockBandGuitar_SoloBlue:   return data->soloBlue;
        case RockBandGuitar_SoloOrange: return data->soloOrange;
        default:                        return false;
        }
    }

    if (type.which_mapping == proto_Output_ghlButton_tag && subtype == LiveGuitar)
    {
        auto data = (const Santroller1GHLGuitar_Data_t *)report;
        switch (type.mapping.ghlButton)
        {
        case GuitarHeroLiveGuitar_Black1:    return data->a;
        case GuitarHeroLiveGuitar_Black2:    return data->b;
        case GuitarHeroLiveGuitar_Black3:    return data->y;
        case GuitarHeroLiveGuitar_White1:    return data->x;
        case GuitarHeroLiveGuitar_White2:    return data->leftShoulder;
        case GuitarHeroLiveGuitar_White3:    return data->rightShoulder;
        case GuitarHeroLiveGuitar_StrumUp:   return up;
        case GuitarHeroLiveGuitar_StrumDown: return down;
        default:                             return false;
        }
    }

    if (type.which_mapping == proto_Output_djhButton_tag && subtype == DjHeroTurntable)
    {
        auto data = (const Santroller1Turntable_Data_t *)report;
        switch (type.mapping.djhButton)
        {
        case DJHTurntable_LeftGreen:  return data->leftGreen;
        case DJHTurntable_LeftRed:    return data->leftRed;
        case DJHTurntable_LeftBlue:   return data->leftBlue;
        case DJHTurntable_RightGreen: return data->rightGreen;
        case DJHTurntable_RightRed:   return data->rightRed;
        case DJHTurntable_RightBlue:  return data->rightBlue;
        default:                      return false;
        }
    }

    return false;
}

uint16_t santroller_v1_tick_analog(const uint8_t *report, SubType subtype, proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        if (subtype == Gamepad)
        {
            auto data = (const Santroller1Gamepad_Data_t *)report;
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:   return (uint16_t)data->leftStickX * 0x101;
            case Gamepad_LeftStickY:   return (uint16_t)(255 - data->leftStickY) * 0x101;
            case Gamepad_RightStickX:  return (uint16_t)data->rightStickX * 0x101;
            case Gamepad_RightStickY:  return (uint16_t)(255 - data->rightStickY) * 0x101;
            case Gamepad_LeftTrigger:  return (uint16_t)data->leftTrigger * 0x101;
            case Gamepad_RightTrigger: return (uint16_t)data->rightTrigger * 0x101;
            default:                   return 0;
            }
        }
    }

    if (type.which_mapping == proto_Output_ghAxis_tag && subtype == GuitarHeroGuitar)
    {
        auto data = (const Santroller1GuitarHeroGuitar_Data_t *)report;
        switch (type.mapping.ghAxis)
        {
        case GuitarHeroGuitar_Whammy: return (uint16_t)data->whammy * 0x101;
        case GuitarHeroGuitar_Tilt:   return (uint16_t)data->tilt * 0x101;
        default:                      return 0;
        }
    }

    if (type.which_mapping == proto_Output_rbAxis_tag && subtype == RockBandGuitar)
    {
        auto data = (const Santroller1RockBandGuitar_Data_t *)report;
        switch (type.mapping.rbAxis)
        {
        case RockBandGuitar_Whammy: return (uint16_t)data->whammy * 0x101;
        case RockBandGuitar_Tilt:   return (uint16_t)data->tilt * 0x101;
        case RockBandGuitar_Pickup: return (uint16_t)data->pickup * 0x101;
        default:                    return 0;
        }
    }

    if (type.which_mapping == proto_Output_ghlAxis_tag && subtype == LiveGuitar)
    {
        auto data = (const Santroller1GHLGuitar_Data_t *)report;
        switch (type.mapping.ghlAxis)
        {
        case GuitarHeroLiveGuitar_Whammy: return (uint16_t)data->whammy * 0x101;
        case GuitarHeroLiveGuitar_Tilt:   return (uint16_t)data->tilt * 0x101;
        default:                          return 0;
        }
    }

    if (type.which_mapping == proto_Output_djhAxis_tag && subtype == DjHeroTurntable)
    {
        auto data = (const Santroller1Turntable_Data_t *)report;
        switch (type.mapping.djhAxis)
        {
        case DJHTurntable_LeftVelocity:  return (uint16_t)data->leftTableVelocity * 0x101;
        case DJHTurntable_RightVelocity: return (uint16_t)data->rightTableVelocity * 0x101;
        case DJHTurntable_Crossfader:    return (uint16_t)data->crossfader * 0x101;
        case DJHTurntable_EffectsKnob:   return (uint16_t)data->effectsKnob * 0x101;
        default:                         return 0;
        }
    }

    return 0;
}
