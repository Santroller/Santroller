#include "wii_extension_decoder.hpp"
#include <string.h>
#include <cmath>
#include "devices/midi.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void WiiExtensionDecoder::reset()
{
    mType = WiiExtType::WiiNoExtension;
    memset(mBuffer, 0, sizeof(mBuffer));
    hiRes = false;
    hasTapBar = false;
    s_box = 0;
}

void WiiExtensionDecoder::decode_id(const uint8_t *id_bytes)
{
    mType = static_cast<WiiExtType>((id_bytes[0] << 8) | id_bytes[5]);
    hiRes = false;
    hasTapBar = false;
    s_box = 0;
}

void WiiExtensionDecoder::update_data(const uint8_t *data, uint8_t len, MidiDevice *midi_device)
{
    uint8_t copy_len = len < sizeof(mBuffer) ? len : sizeof(mBuffer);
    memcpy(mBuffer, data, copy_len);

    if (s_box)
    {
        for (int i = 0; i < copy_len; i++)
        {
            mBuffer[i] = (uint8_t)(((mBuffer[i] ^ s_box) + s_box) & 0xFF);
        }
    }

    if (mType == WiiExtType::WiiGuitarHeroDrums && midi_device)
    {
        uint8_t velocity = ((mBuffer[4] & 0b00000001) |
                            ((mBuffer[4] & 0b10000000) >> 6) |
                            ((mBuffer[3] & 0b00000001) << 2) |
                            ((mBuffer[2] & 0b00000001) << 3) |
                            ((mBuffer[3] & (0b11100000)) >> 1));
        uint8_t note = (mBuffer[2] >> 1) & 0x7f;
        uint8_t channel = ((~mBuffer[3]) >> 1) & 0xF;
        velocity = 0x7F - velocity;
        note = 0x7F - note;
        if (velocity || note)
        {
            uint8_t packet[] = {0, (uint8_t)(MIDI_CIN_NOTE_ON << 4 | channel), note, velocity};
            midi_device->process_midi_data(packet, sizeof(packet));
        }
    }

    if (mType == WiiExtType::WiiGuitarHeroGuitar)
    {
        auto lastTapWii = (mBuffer[2] & 0x1f);
        if (!hasTapBar)
        {
            if (lastTapWii == 0x0F)
            {
                hasTapBar = true;
            }
        }
    }
}

SubType WiiExtensionDecoder::get_subtype() const
{
    switch (mType)
    {
    case WiiExtType::WiiGuitarHeroGuitar:
        return SubType_GuitarHeroGuitar;
    case WiiExtType::WiiGuitarHeroDrums:
        return SubType_GuitarHeroDrums;
    case WiiExtType::WiiDjHeroTurntable:
        return SubType_DjHeroTurntable;
    case WiiExtType::WiiTaikoNoTatsujinController:
        return SubType_Taiko;
    case WiiExtType::WiiClassicController:
    case WiiExtType::WiiClassicControllerPro:
    case WiiExtType::WiiNunchuk:
    case WiiExtType::WiiThqUdrawTablet:
    case WiiExtType::WiiUbisoftDrawsomeTablet:
    case WiiExtType::WiiNoExtension:
    default:
        return SubType_Gamepad;
    }
}

uint16_t WiiExtensionDecoder::read_axis(proto_WiiAxisType type) const
{
    switch (mType)
    {
    case WiiExtType::WiiClassicControllerPro:
    case WiiExtType::WiiClassicController:
    {
        if (hiRes)
        {
            switch (type)
            {
            case WiiAxisType::WiiAxisClassicLeftStickX:
                return (mBuffer[0]) << 8;
            case WiiAxisType::WiiAxisClassicLeftStickY:
                return ((mBuffer[2]) << 8);
            case WiiAxisType::WiiAxisClassicRightStickX:
                return (mBuffer[1]) << 8;
            case WiiAxisType::WiiAxisClassicRightStickY:
                return ((mBuffer[3]) << 8);
            case WiiAxisType::WiiAxisClassicLeftTrigger:
                if (mType == WiiClassicControllerPro)
                {
                    return read_button(WiiButtonClassicZl) ? 65535 : 0;
                }
                return mBuffer[4] << 8;
            case WiiAxisType::WiiAxisClassicRightTrigger:
                if (mType == WiiClassicControllerPro)
                {
                    return read_button(WiiButtonClassicZr) ? 65535 : 0;
                }
                return mBuffer[5] << 8;
            default:
                return 0;
            }
        }
        else
        {
            switch (type)
            {
            case WiiAxisType::WiiAxisClassicLeftStickX:
                return ((mBuffer[0] & 0x3f)) << 10;
            case WiiAxisType::WiiAxisClassicLeftStickY:
                return (((mBuffer[1] & 0x3f)) << 10);
            case WiiAxisType::WiiAxisClassicRightStickX:
                return ((((mBuffer[0] & 0xc0) >> 3) | ((mBuffer[1] & 0xc0) >> 5) | (mBuffer[2] >> 7))) << 11;
            case WiiAxisType::WiiAxisClassicRightStickY:
                return (((mBuffer[2] & 0x1f)) << 11);
            case WiiAxisType::WiiAxisClassicLeftTrigger:
                if (mType == WiiClassicControllerPro)
                {
                    return read_button(WiiButtonClassicZl) ? 65535 : 0;
                }
                return (((mBuffer[3] & 0xE0) >> 5 | (mBuffer[2] & 0x60) >> 2)) << 11;
            case WiiAxisType::WiiAxisClassicRightTrigger:
                if (mType == WiiClassicControllerPro)
                {
                    return read_button(WiiButtonClassicZr) ? 65535 : 0;
                }
                return (mBuffer[3] & 0x1f) << 11;
            default:
                return 0;
            }
        }
        break;
    }
    case WiiExtType::WiiDjHeroTurntable:
    {
        switch (type)
        {
        case WiiAxisType::WiiAxisDjCrossfadeSlider:
            return ((mBuffer[2] & 0x1E) >> 1) << 12;
        case WiiAxisType::WiiAxisDjEffectDial:
            return (((mBuffer[3] & 0xE0) >> 5 | (mBuffer[2] & 0x60) >> 2)) << 11;
        case WiiAxisType::WiiAxisDjStickX:
            return ((mBuffer[0] & 0x3F)) << 10;
        case WiiAxisType::WiiAxisDjStickY:
            return ((mBuffer[1] & 0x3F)) << 10;
        case WiiAxisType::WiiAxisDjTurntableLeft:
            return ((mBuffer[4] & 1) ? 32 + (0x1F - (mBuffer[3] & 0x1F)) : 32 - (mBuffer[3] & 0x1F)) << 10;
        case WiiAxisType::WiiAxisDjTurntableRight:
        {
            uint8_t rtt = (mBuffer[2] & 0x80) >> 7 | (mBuffer[1] & 0xC0) >> 5 | (mBuffer[0] & 0xC0) >> 3;
            return ((mBuffer[2] & 1) ? 32 + (0x1F - rtt) : 32 - rtt);
        }
        default:
            return 0;
        }
        break;
    }
    case WiiExtType::WiiUbisoftDrawsomeTablet:
    {
        switch (type)
        {
        case WiiAxisType::WiiAxisDrawsomePenPressure:
            return (mBuffer[4] | (mBuffer[5] & 0x0f) << 8);
        case WiiAxisType::WiiAxisDrawsomePenX:
            return (mBuffer[0] | mBuffer[1] << 8);
        case WiiAxisType::WiiAxisDrawsomePenY:
            return (mBuffer[2] | mBuffer[3] << 8);
        default:
            return 0;
        }
        break;
    }
    case WiiExtType::WiiThqUdrawTablet:
    {
        switch (type)
        {
        case WiiAxisType::WiiAxisUDrawPenPressure:
            return (mBuffer[3]);
        case WiiAxisType::WiiAxisUDrawPenX:
            return ((mBuffer[2] & 0x0f) << 8) | mBuffer[0];
        case WiiAxisType::WiiAxisUDrawPenY:
            return ((mBuffer[2] & 0xf0) << 4) | mBuffer[1];
        default:
            return 0;
        }
        break;
    }
    case WiiExtType::WiiGuitarHeroGuitar:
    {
        switch (type)
        {
        case WiiAxisType::WiiAxisGuitarJoystickX:
            return ((mBuffer[0] & 0x3f)) << 10;
        case WiiAxisType::WiiAxisGuitarJoystickY:
            return ((mBuffer[1] & 0x3f)) << 10;
        case WiiAxisType::WiiAxisGuitarWhammy:
            return (mBuffer[3] & 0x1f) << 11;
        default:
            return 0;
        }
        break;
    }
    case WiiExtType::WiiGuitarHeroDrums:
    {
        switch (type)
        {
        case WiiAxisType::WiiAxisDrumJoystickX:
            return ((mBuffer[0] & 0x3f)) << 10;
        case WiiAxisType::WiiAxisDrumJoystickY:
            return ((mBuffer[1] & 0x3f)) << 10;
        default:
            return 0;
        }
        break;
    }
    case WiiExtType::WiiNunchuk:
    {
        switch (type)
        {
        case WiiAxisType::WiiAxisNunchukAccelerationX:
            return ((mBuffer[2] << 2) | ((mBuffer[5] & 0xC0) >> 6)) << 6;
        case WiiAxisType::WiiAxisNunchukAccelerationY:
            return ((mBuffer[3] << 2) | ((mBuffer[5] & 0x30) >> 4)) << 6;
        case WiiAxisType::WiiAxisNunchukAccelerationZ:
            return ((mBuffer[4] << 2) | ((mBuffer[5] & 0xC) >> 2)) << 6;
        case WiiAxisType::WiiAxisNunchukRotationPitch:
            return (std::atan2(((mBuffer[3] << 2) | ((mBuffer[5] & 0x30) >> 4)) - 511.0, ((mBuffer[4] << 2) | ((mBuffer[5] & 0xC) >> 2)) - 511.0) * 32767 / M_PI) + 32767;
        case WiiAxisType::WiiAxisNunchukRotationRoll:
            return (-std::atan2(((mBuffer[2] << 2) | ((mBuffer[5] & 0xC0) >> 6)) - 511.0, ((mBuffer[4] << 2) | ((mBuffer[5] & 0xC) >> 2)) - 511.0) * 32767 / M_PI) + 32767;
        case WiiAxisType::WiiAxisNunchukStickX:
            return (mBuffer[0]) << 8;
        case WiiAxisType::WiiAxisNunchukStickY:
            return (mBuffer[1]) << 8;
        default:
            return 0;
        }
    }
    default:
        return 0;
    }
    return 0;
}

bool WiiExtensionDecoder::read_button(proto_WiiButtonType type) const
{
    auto lastTap = hasTapBar ? (mBuffer[2] & 0x1f) : 0x0F;
    auto wiiButtonsLow = ~mBuffer[4];
    auto wiiButtonsHigh = ~mBuffer[5];
    if (hiRes)
    {
        wiiButtonsLow = ~mBuffer[6];
        wiiButtonsHigh = ~mBuffer[7];
    }
    switch (mType)
    {
    case WiiExtType::WiiClassicControllerPro:
    case WiiExtType::WiiClassicController:
    {
        switch (type)
        {
        case WiiButtonClassicRt:
            return ((wiiButtonsLow) & (1 << 1));
        case WiiButtonClassicPlus:
            return ((wiiButtonsLow) & (1 << 2));
        case WiiButtonClassicHome:
            return ((wiiButtonsLow) & (1 << 3));
        case WiiButtonClassicMinus:
            return ((wiiButtonsLow) & (1 << 4));
        case WiiButtonClassicLt:
            return ((wiiButtonsLow) & (1 << 5));
        case WiiButtonClassicDPadDown:
            return ((wiiButtonsLow) & (1 << 6));
        case WiiButtonClassicDPadRight:
            return ((wiiButtonsLow) & (1 << 7));
        case WiiButtonClassicDPadUp:
            return ((wiiButtonsHigh) & (1 << 0));
        case WiiButtonClassicDPadLeft:
            return ((wiiButtonsHigh) & (1 << 1));
        case WiiButtonClassicZr:
            if (mType == WiiClassicControllerPro)
            {
                return ((wiiButtonsLow) & (1 << 1));
            }
            return ((wiiButtonsHigh) & (1 << 2));
        case WiiButtonClassicX:
            return ((wiiButtonsHigh) & (1 << 3));
        case WiiButtonClassicA:
            return ((wiiButtonsHigh) & (1 << 4));
        case WiiButtonClassicY:
            return ((wiiButtonsHigh) & (1 << 5));
        case WiiButtonClassicB:
            return ((wiiButtonsHigh) & (1 << 6));
        case WiiButtonClassicZl:
            if (mType == WiiClassicControllerPro)
            {
                return ((wiiButtonsLow) & (1 << 5));
            }
            return ((wiiButtonsHigh) & (1 << 7));
        default:
            return false;
        }
        break;
    }

    case WiiExtType::WiiDjHeroTurntable:
    {
        switch (type)
        {
        case WiiButtonDjHeroPlus:
            return ((wiiButtonsLow) & (1 << 2));
        case WiiButtonDjHeroMinus:
            return ((wiiButtonsLow) & (1 << 4));
        case WiiButtonDjHeroLeftBlue:
            return ((wiiButtonsHigh) & (1 << 7));
        case WiiButtonDjHeroLeftRed:
            return ((wiiButtonsLow) & (1 << 5));
        case WiiButtonDjHeroLeftGreen:
            return ((wiiButtonsHigh) & (1 << 3));
        case WiiButtonDjHeroRightGreen:
            return ((wiiButtonsHigh) & (1 << 5));
        case WiiButtonDjHeroRightRed:
            return ((wiiButtonsLow) & (1 << 1));
        case WiiButtonDjHeroRightBlue:
            return ((wiiButtonsHigh) & (1 << 2));
        case WiiButtonDjHeroEuphoria:
            return ((wiiButtonsHigh) & (1 << 4));
        default:
            return false;
        }
        break;
    }

    case WiiExtType::WiiGuitarHeroDrums:
    {
        switch (type)
        {
        case WiiButtonDrumPlus:
            return ((wiiButtonsLow) & (1 << 2));
        case WiiButtonDrumMinus:
            return ((wiiButtonsLow) & (1 << 4));
        default:
            return false;
        }
        break;
    }

    case WiiExtType::WiiGuitarHeroGuitar:
    {
        switch (type)
        {
        case WiiButtonGuitarPlus:
            return ((wiiButtonsLow) & (1 << 2));
        case WiiButtonGuitarMinus:
            return ((wiiButtonsLow) & (1 << 4));
        case WiiButtonGuitarStrumDown:
            return ((wiiButtonsLow) & (1 << 6));
        case WiiButtonGuitarStrumUp:
            return ((wiiButtonsHigh) & (1 << 0));
        case WiiButtonGuitarGreen:
            return ((wiiButtonsHigh) & (1 << 4));
        case WiiButtonGuitarRed:
            return ((wiiButtonsHigh) & (1 << 6));
        case WiiButtonGuitarYellow:
            return ((wiiButtonsHigh) & (1 << 3));
        case WiiButtonGuitarBlue:
            return ((wiiButtonsHigh) & (1 << 5));
        case WiiButtonGuitarOrange:
            return ((wiiButtonsHigh) & (1 << 7));
        case WiiButtonGuitarTapGreen:
            return lastTap < 0x0A;
        case WiiButtonGuitarTapRed:
            return lastTap != 0x0F && lastTap < 0x12 && lastTap >= 0x0A;
        case WiiButtonGuitarTapYellow:
            return lastTap < 0x17 && lastTap >= 0x12;
        case WiiButtonGuitarTapBlue:
            return lastTap < 0x1F && lastTap >= 0x17;
        case WiiButtonGuitarTapOrange:
            return lastTap >= 0x1F;
        case WiiButtonGuitarPedal:
            return ((wiiButtonsHigh) & (1 << 2));
        default:
            return false;
        }
        break;
    }
    case WiiExtType::WiiNunchuk:
    {
        switch (type)
        {
        case WiiButtonNunchukC:
            return ((wiiButtonsHigh) & (1 << 1));
        case WiiButtonNunchukZ:
            return ((wiiButtonsHigh) & (1 << 0));
        default:
            return false;
        }
        break;
    }

    case WiiExtType::WiiTaikoNoTatsujinController:
    {
        switch (type)
        {
        case WiiButtonTaTaConRightDrumRim:
            return ((~mBuffer[0]) & (1 << 3));
        case WiiButtonTaTaConRightDrumCenter:
            return ((~mBuffer[0]) & (1 << 4));
        case WiiButtonTaTaConLeftDrumRim:
            return ((~mBuffer[0]) & (1 << 5));
        case WiiButtonTaTaConLeftDrumCenter:
            return ((~mBuffer[0]) & (1 << 6));
        default:
            return false;
        }
        break;
    }

    case WiiExtType::WiiUbisoftDrawsomeTablet:
    {
        switch (type)
        {
        case WiiButtonUDrawPenButton1:
            return ((wiiButtonsHigh) & (1 << 0));
        case WiiButtonUDrawPenButton2:
            return ((wiiButtonsHigh) & (1 << 1));
        case WiiButtonUDrawPenClick:
            return ((~wiiButtonsHigh) & (1 << 2));
        default:
            return false;
        }
        break;
    }
    default:
        return false;
    }
    return false;
}

bool WiiExtensionDecoder::tick_digital(proto_Output &type) const
{
    if (type.which_mapping == proto_Output_ghButton_tag)
    {
        if (mType == WiiExtType::WiiGuitarHeroGuitar)
        {
            switch (type.mapping.ghButton)
            {
            case GuitarHeroGuitar_Green:     return read_button(WiiButtonGuitarGreen) || read_button(WiiButtonGuitarTapGreen);
            case GuitarHeroGuitar_Red:       return read_button(WiiButtonGuitarRed) || read_button(WiiButtonGuitarTapRed);
            case GuitarHeroGuitar_Yellow:    return read_button(WiiButtonGuitarYellow) || read_button(WiiButtonGuitarTapYellow);
            case GuitarHeroGuitar_Blue:      return read_button(WiiButtonGuitarBlue) || read_button(WiiButtonGuitarTapBlue);
            case GuitarHeroGuitar_Orange:    return read_button(WiiButtonGuitarOrange) || read_button(WiiButtonGuitarTapOrange);
            case GuitarHeroGuitar_TapGreen:  return read_button(WiiButtonGuitarTapGreen);
            case GuitarHeroGuitar_TapRed:    return read_button(WiiButtonGuitarTapRed);
            case GuitarHeroGuitar_TapYellow: return read_button(WiiButtonGuitarTapYellow);
            case GuitarHeroGuitar_TapBlue:   return read_button(WiiButtonGuitarTapBlue);
            case GuitarHeroGuitar_TapOrange: return read_button(WiiButtonGuitarTapOrange);
            case GuitarHeroGuitar_Pedal:     return read_button(WiiButtonGuitarPedal);
            default:                         return false;
            }
        }
    }

    if (type.which_mapping == proto_Output_rbButton_tag)
    {
        if (mType == WiiExtType::WiiGuitarHeroGuitar)
        {
            switch (type.mapping.rbButton)
            {
            case RockBandGuitar_Green:       return read_button(WiiButtonGuitarGreen) || read_button(WiiButtonGuitarTapGreen);
            case RockBandGuitar_Red:         return read_button(WiiButtonGuitarRed) || read_button(WiiButtonGuitarTapRed);
            case RockBandGuitar_Yellow:      return read_button(WiiButtonGuitarYellow) || read_button(WiiButtonGuitarTapYellow);
            case RockBandGuitar_Blue:        return read_button(WiiButtonGuitarBlue) || read_button(WiiButtonGuitarTapBlue);
            case RockBandGuitar_Orange:      return read_button(WiiButtonGuitarOrange) || read_button(WiiButtonGuitarTapOrange);
            case RockBandGuitar_SoloGreen:   return read_button(WiiButtonGuitarTapGreen);
            case RockBandGuitar_SoloRed:     return read_button(WiiButtonGuitarTapRed);
            case RockBandGuitar_SoloYellow:  return read_button(WiiButtonGuitarTapYellow);
            case RockBandGuitar_SoloBlue:    return read_button(WiiButtonGuitarTapBlue);
            case RockBandGuitar_SoloOrange:  return read_button(WiiButtonGuitarTapOrange);
            case RockBandGuitar_Pedal:       return read_button(WiiButtonGuitarPedal);
            default:                         return false;
            }
        }
    }

    if (type.which_mapping == proto_Output_djhButton_tag)
    {
        if (mType == WiiExtType::WiiDjHeroTurntable)
        {
            switch (type.mapping.djhButton)
            {
            case DJHTurntable_LeftGreen:  return read_button(WiiButtonDjHeroLeftGreen);
            case DJHTurntable_LeftRed:    return read_button(WiiButtonDjHeroLeftRed);
            case DJHTurntable_LeftBlue:   return read_button(WiiButtonDjHeroLeftBlue);
            case DJHTurntable_RightGreen: return read_button(WiiButtonDjHeroRightGreen);
            case DJHTurntable_RightRed:   return read_button(WiiButtonDjHeroRightRed);
            case DJHTurntable_RightBlue:  return read_button(WiiButtonDjHeroRightBlue);
            default:                      return false;
            }
        }
    }

    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (mType)
        {
        case WiiExtType::WiiClassicController:
        case WiiExtType::WiiClassicControllerPro:
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:             return read_button(WiiButtonClassicA);
            case Gamepad_B:             return read_button(WiiButtonClassicB);
            case Gamepad_X:             return read_button(WiiButtonClassicX);
            case Gamepad_Y:             return read_button(WiiButtonClassicY);
            case Gamepad_DpadUp:        return read_button(WiiButtonClassicDPadUp);
            case Gamepad_DpadDown:      return read_button(WiiButtonClassicDPadDown);
            case Gamepad_DpadLeft:      return read_button(WiiButtonClassicDPadLeft);
            case Gamepad_DpadRight:     return read_button(WiiButtonClassicDPadRight);
            case Gamepad_LeftShoulder:  return read_button(WiiButtonClassicLt);
            case Gamepad_RightShoulder: return read_button(WiiButtonClassicRt);
            case Gamepad_Start:         return read_button(WiiButtonClassicPlus);
            case Gamepad_Back:          return read_button(WiiButtonClassicMinus);
            case Gamepad_Guide:         return read_button(WiiButtonClassicHome);
            default:                    return false;
            }

        case WiiExtType::WiiNunchuk:
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_LeftShoulder: return read_button(WiiButtonNunchukC);
            default:                   return false;
            }

        case WiiExtType::WiiGuitarHeroGuitar:
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:             return read_button(WiiButtonGuitarGreen) || read_button(WiiButtonGuitarTapGreen);
            case Gamepad_B:             return read_button(WiiButtonGuitarRed) || read_button(WiiButtonGuitarTapRed);
            case Gamepad_Y:             return read_button(WiiButtonGuitarYellow) || read_button(WiiButtonGuitarTapYellow);
            case Gamepad_X:             return read_button(WiiButtonGuitarBlue) || read_button(WiiButtonGuitarTapBlue);
            case Gamepad_LeftShoulder:  return read_button(WiiButtonGuitarOrange) || read_button(WiiButtonGuitarTapOrange);
            case Gamepad_DpadUp:        return read_button(WiiButtonGuitarStrumUp);
            case Gamepad_DpadDown:      return read_button(WiiButtonGuitarStrumDown);
            case Gamepad_Back:          return read_button(WiiButtonGuitarMinus);
            case Gamepad_Start:         return read_button(WiiButtonGuitarPlus);
            default:                    return false;
            }

        case WiiExtType::WiiGuitarHeroDrums:
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_Start: return read_button(WiiButtonDrumPlus);
            case Gamepad_Back:  return read_button(WiiButtonDrumMinus);
            default:            return false;
            }

        case WiiExtType::WiiDjHeroTurntable:
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_Start: return read_button(WiiButtonDjHeroPlus);
            case Gamepad_Back:  return read_button(WiiButtonDjHeroMinus);
            case Gamepad_X:     return read_button(WiiButtonDjHeroEuphoria);
            default:            return false;
            }

        case WiiExtType::WiiTaikoNoTatsujinController:
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_DpadLeft:  return read_button(WiiButtonTaTaConLeftDrumRim);
            case Gamepad_DpadDown:  return read_button(WiiButtonTaTaConLeftDrumCenter);
            case Gamepad_A:         return read_button(WiiButtonTaTaConRightDrumCenter);
            case Gamepad_B:         return read_button(WiiButtonTaTaConRightDrumRim);
            default:                return false;
            }

        case WiiExtType::WiiUbisoftDrawsomeTablet:
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A: return read_button(WiiButtonUDrawPenButton1);
            case Gamepad_B: return read_button(WiiButtonUDrawPenButton2);
            case Gamepad_X: return read_button(WiiButtonUDrawPenClick);
            default:        return false;
            }

        default:
            return false;
        }
    }

    return false;
}

uint16_t WiiExtensionDecoder::tick_analog(proto_Output &type) const
{
    if (type.which_mapping == proto_Output_ghAxis_tag)
    {
        if (mType == WiiExtType::WiiGuitarHeroGuitar)
        {
            switch (type.mapping.ghAxis)
            {
            case GuitarHeroGuitar_Whammy: return read_axis(WiiAxisGuitarWhammy);
            default:                      return 0;
            }
        }
    }

    if (type.which_mapping == proto_Output_rbAxis_tag)
    {
        if (mType == WiiExtType::WiiGuitarHeroGuitar)
        {
            switch (type.mapping.rbAxis)
            {
            case RockBandGuitar_Whammy: return read_axis(WiiAxisGuitarWhammy);
            default:                    return 0;
            }
        }
    }

    if (type.which_mapping == proto_Output_djhAxis_tag)
    {
        if (mType == WiiExtType::WiiDjHeroTurntable)
        {
            switch (type.mapping.djhAxis)
            {
            case DJHTurntable_LeftVelocity:  return read_axis(WiiAxisDjTurntableLeft);
            case DJHTurntable_RightVelocity: return read_axis(WiiAxisDjTurntableRight);
            case DJHTurntable_Crossfader:    return read_axis(WiiAxisDjCrossfadeSlider);
            case DJHTurntable_EffectsKnob:   return read_axis(WiiAxisDjEffectDial);
            default:                         return 0;
            }
        }
    }

    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        switch (mType)
        {
        case WiiExtType::WiiClassicController:
        case WiiExtType::WiiClassicControllerPro:
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:   return read_axis(WiiAxisClassicLeftStickX);
            case Gamepad_LeftStickY:   return read_axis(WiiAxisClassicLeftStickY);
            case Gamepad_RightStickX:  return read_axis(WiiAxisClassicRightStickX);
            case Gamepad_RightStickY:  return read_axis(WiiAxisClassicRightStickY);
            case Gamepad_LeftTrigger:  return read_axis(WiiAxisClassicLeftTrigger);
            case Gamepad_RightTrigger: return read_axis(WiiAxisClassicRightTrigger);
            default:                   return 0;
            }

        case WiiExtType::WiiNunchuk:
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:   return read_axis(WiiAxisNunchukStickX);
            case Gamepad_LeftStickY:   return read_axis(WiiAxisNunchukStickY);
            case Gamepad_LeftTrigger:  return read_button(WiiButtonNunchukZ) ? 65535 : 0;
            default:                   return 0;
            }

        case WiiExtType::WiiGuitarHeroGuitar:
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:   return read_axis(WiiAxisGuitarJoystickX);
            case Gamepad_LeftStickY:   return read_axis(WiiAxisGuitarJoystickY);
            case Gamepad_RightTrigger: return read_axis(WiiAxisGuitarWhammy);
            default:                   return 0;
            }

        case WiiExtType::WiiGuitarHeroDrums:
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX: return read_axis(WiiAxisDrumJoystickX);
            case Gamepad_LeftStickY: return read_axis(WiiAxisDrumJoystickY);
            default:                 return 0;
            }

        case WiiExtType::WiiDjHeroTurntable:
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX: return read_axis(WiiAxisDjStickX);
            case Gamepad_LeftStickY: return read_axis(WiiAxisDjStickY);
            default:                 return 0;
            }

        case WiiExtType::WiiUbisoftDrawsomeTablet:
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:   return read_axis(WiiAxisDrawsomePenX);
            case Gamepad_LeftStickY:   return read_axis(WiiAxisDrawsomePenY);
            case Gamepad_RightTrigger: return read_axis(WiiAxisDrawsomePenPressure);
            default:                   return 0;
            }

        case WiiExtType::WiiThqUdrawTablet:
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:   return read_axis(WiiAxisUDrawPenX);
            case Gamepad_LeftStickY:   return read_axis(WiiAxisUDrawPenY);
            case Gamepad_RightTrigger: return read_axis(WiiAxisUDrawPenPressure);
            default:                   return 0;
            }

        default:
            return 0;
        }
    }

    return 0;
}
