#include "gip_button_mapping.h"
#include <stdint.h>
#include "device.pb.h"

bool gip_tick_digital(const void *input_data, uint8_t subtype, proto_Output *type)
{
    if (!input_data || !type) {
        return false;
    }
    
    // Handle gamepad buttons (common to all device types)
    if (type->which_mapping == proto_Output_gamepadButton_tag)
    {
        auto data = (const XboxOneGamepad_Data_t *)input_data;
        switch (type->mapping.gamepadButton)
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
    
    // Handle device-specific buttons
    switch (subtype)
    {
    case SubType_RockBandGuitar:
        if (type->which_mapping == proto_Output_rbButton_tag)
        {
            auto data = (const XboxOneRockBandGuitar_Data_t *)input_data;
            switch (type->mapping.rbButton)
            {
            case RockBandGuitar_Green:
                return data->green;
            case RockBandGuitar_Red:
                return data->red;
            case RockBandGuitar_Yellow:
                return data->yellow;
            case RockBandGuitar_Blue:
                return data->blue;
            case RockBandGuitar_Orange:
                return data->orange;
            case RockBandGuitar_SoloGreen:
                return data->soloGreen;
            case RockBandGuitar_SoloRed:
                return data->soloRed;
            case RockBandGuitar_SoloYellow:
                return data->soloYellow;
            case RockBandGuitar_SoloBlue:
                return data->soloBlue;
            case RockBandGuitar_SoloOrange:
                return data->soloOrange;
            default:
                return false;
            }
        }
        return false;
        
    case SubType_RockBandDrums:
        if (type->which_mapping == proto_Output_rbDrumButton_tag)
        {
            auto data = (const XboxOneRockBandDrums_Data_t *)input_data;
            switch (type->mapping.rbDrumButton)
            {
                case RockBandDrums_Kick1Pedal:
                    return data->leftShoulder;
                case RockBandDrums_Kick2Pedal:
                    return data->rightShoulder;
            default:
                return false;
            }
        }
        return false;
        
    case SubType_LiveGuitar:
        if (type->which_mapping == proto_Output_ghlButton_tag)
        {
            auto data = (const XboxOneGHLGuitar_Data_t *)input_data;
            switch (type->mapping.ghlButton)
            {
            case GuitarHeroLiveGuitar_Black1:
                return data->report.a;
            case GuitarHeroLiveGuitar_Black2:
                return data->report.b;
            case GuitarHeroLiveGuitar_Black3:
                return data->report.y;
            case GuitarHeroLiveGuitar_White1:
                return data->report.x;
            case GuitarHeroLiveGuitar_White2:
                return data->report.leftShoulder;
            case GuitarHeroLiveGuitar_White3:
                return data->report.rightShoulder;
            case GuitarHeroLiveGuitar_StrumUp:
                return data->report.strumBar == 0x00;
            case GuitarHeroLiveGuitar_StrumDown:
                return data->report.strumBar == 0xFF;
            default:
                return false;
            }
        }
        return false;
        
    default:
        return false;
    }
    
    return false;
}

uint16_t gip_tick_analog(const void *input_data, uint8_t subtype, proto_Output *type)
{
    if (!input_data || !type) {
        return 0;
    }
    
    switch (subtype)
    {
    case SubType_LiveGuitar:
        if (type->which_mapping == proto_Output_ghlAxis_tag)
        {
            auto data = (const XboxOneGHLGuitar_Data_t *)input_data;
            switch (type->mapping.ghlAxis)
            {
            case GuitarHeroLiveGuitar_Whammy:
                return data->report.whammy << 8;
            case GuitarHeroLiveGuitar_Tilt:
                return data->report.tilt << 2;
            default:
                return 0;
            }
        }
        break;
        
    case SubType_RockBandDrums:
        if (type->which_mapping == proto_Output_rbDrumAxis_tag)
        {
            auto data = (const XboxOneRockBandDrums_Data_t *)input_data;
            switch (type->mapping.rbDrumAxis)
            {
            case RockBandDrums_GreenPad:
                return data->greenVelocity << 12;
            case RockBandDrums_RedPad:
                return data->redVelocity << 12;
            case RockBandDrums_YellowPad:
                return data->yellowVelocity << 12;
            case RockBandDrums_BluePad:
                return data->blueVelocity << 12;
            case RockBandDrums_GreenCymbal:
                return data->greenCymbalVelocity << 12;
            case RockBandDrums_YellowCymbal:
                return data->yellowCymbalVelocity << 12;
            case RockBandDrums_BlueCymbal:
                return data->blueCymbalVelocity << 12;
            default:
                return 0;
            }
        }
        break;
    case SubType_RockBandGuitar:
        if (type->which_mapping == proto_Output_rbAxis_tag)
        {
            auto data = (const XboxOneRockBandGuitar_Data_t *)input_data;
            switch (type->mapping.rbAxis)
            {
            case RockBandGuitar_Whammy:
                return data->whammy + INT16_MAX;
            case RockBandGuitar_Tilt:
                return data->tilt + INT16_MAX;
            case RockBandGuitar_Pickup:
                return data->pickup + INT16_MAX;
            default:
                return 0;
            }
        }
        break;
        
    default:
        // Handle gamepad axes (common to all device types)
        if (type->which_mapping == proto_Output_gamepadAxis_tag)
        {
            auto data = (const XboxOneGamepad_Data_t *)input_data;
            switch (type->mapping.gamepadAxis)
            {
            case Gamepad_LeftTrigger:
                return data->leftTrigger << 8;
            case Gamepad_RightTrigger:
                return data->rightTrigger << 8;
            case Gamepad_LeftStickX:
                return data->leftStickX + INT16_MAX;
            case Gamepad_LeftStickY:
                return data->leftStickY + INT16_MAX;
            case Gamepad_RightStickX:
                return data->rightStickX + INT16_MAX;
            case Gamepad_RightStickY:
                return data->rightStickY + INT16_MAX;
            default:
                return 0;
            }
        }
        break;
    }
    
    return 0;
}
