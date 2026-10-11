#include "protocols/santroller_v1.hpp"
#include "protocols/pro_keys.hpp"
#include "devices/usb/host/gh_slider_helpers.h"

// Decoding for Santroller 1 HID reports (report id 1, layout depends on the subtype),
// shared by the BLE and USB host implementations. The layouts are the Santroller1*_Data_t
// structs in santroller_v1.hpp, but they are decoded with lookup tables here instead, as
// that is a lot smaller than a switch per subtype.
// Every Santroller 1 report has its dpad at byte 3 as a hat (0..7, 8=neutral). This
// includes dance pads, only Santroller 2 reports their dpad as a bitmask.

namespace
{
// Buttons are numbered by bit, starting at bit 0 of byte 1, so byte 2 holds 8 - 15
constexpr uint8_t NONE = 0xFF;

// Indexed by GamepadButtonType - 1: A, B, X, Y, Back, Start, Guide, Capture, LB, RB, L3, R3
constexpr uint8_t GAMEPAD_BUTTON_COUNT = 12;
const uint8_t gamepad_buttons[GAMEPAD_BUTTON_COUNT] = {1, 2, 0, 3, 8, 9, 12, 13, 4, 5, 10, 11};
const uint8_t stage_kit_buttons[GAMEPAD_BUTTON_COUNT] = {0, 1, 2, 3, 8, 9, 12, 13, 4, 5, 10, 11};
const uint8_t gh_guitar_buttons[GAMEPAD_BUTTON_COUNT] = {0, 1, 3, 2, 6, 7, 8, NONE, 4, 5, NONE, NONE};
const uint8_t rb_guitar_buttons[GAMEPAD_BUTTON_COUNT] = {0, 1, 3, 2, 10, 11, 12, NONE, 4, NONE, NONE, NONE};
// The solo frets also press the matching face button
const uint8_t rb_guitar_solo_buttons[GAMEPAD_BUTTON_COUNT] = {5, 6, 8, 7, NONE, NONE, NONE, NONE, 9, NONE, NONE, NONE};
const uint8_t ghl_guitar_buttons[GAMEPAD_BUTTON_COUNT] = {0, 1, 3, 2, 6, 7, 9, NONE, 4, 5, 8, NONE};
const uint8_t gh_drum_buttons[GAMEPAD_BUTTON_COUNT] = {0, 1, 3, 2, 6, 7, 8, NONE, 5, 4, NONE, NONE};
const uint8_t rb_drum_buttons[GAMEPAD_BUTTON_COUNT] = {0, 1, 2, 3, 8, 9, 10, NONE, 6, 7, NONE, NONE};
const uint8_t turntable_buttons[GAMEPAD_BUTTON_COUNT] = {0, 1, 2, 3, 4, 5, 6, NONE, NONE, NONE, NONE, NONE};
const uint8_t pro_buttons[GAMEPAD_BUTTON_COUNT] = {1, 2, 0, 3, 4, 5, 6, NONE, NONE, NONE, NONE, NONE};

// Indexed by GuitarHeroLiveGuitarButtonType - 1: White1-3, Black1-3, GHTV
const uint8_t ghl_frets[] = {3, 4, 5, 0, 1, 2, 8};

// Axes are byte offsets into the report, 0 for none
// Indexed by GamepadAxisType - 1: left x, left y, right x, right y, left trigger, right trigger
const uint8_t gamepad_axes[] = {4, 5, 6, 7, 8, 9};
// Indexed by GuitarHeroDrumsAxisType - 1: red, yellow, blue, orange, green, kick
const uint8_t gh_drum_axes[] = {5, 6, 7, 8, 4, 9};
// Indexed by RockBandDrumsAxisType - 1: red, yellow, blue, green pad, yellow, blue, green cymbal
const uint8_t rb_drum_axes[] = {5, 6, 7, 4, 9, 10, 8};
// Indexed by GuitarHeroGuitarAxisType - 1: whammy, tilt
const uint8_t gh_guitar_axes[] = {4, 6};
// Indexed by RockBandGuitarAxisType - 1: whammy, tilt, pickup
const uint8_t rb_guitar_axes[] = {4, 6, 5};
// Indexed by GuitarHeroLiveGuitarAxisType - 1: whammy, tilt
const uint8_t ghl_guitar_axes[] = {4, 5};
// Indexed by DJHTurntableAxisType - 1: left, right velocity, effects knob, crossfader
const uint8_t turntable_axes[] = {4, 5, 6, 7};

template <size_t N>
uint8_t lookup(const uint8_t (&table)[N], int value)
{
    return (value >= 1 && value <= (int)N) ? table[value - 1] : NONE;
}

bool button(const uint8_t *report, uint8_t bit)
{
    return bit != NONE && ((report[1 + (bit >> 3)] >> (bit & 7)) & 1);
}

template <size_t N>
uint16_t axis(const uint8_t *report, const uint8_t (&table)[N], int value)
{
    uint8_t offset = lookup(table, value);
    return offset == NONE ? 0 : (uint16_t)report[offset] * 0x101;
}

const uint8_t *gamepad_button_table(SubType subtype)
{
    switch (subtype)
    {
    case GuitarHeroGuitar: return gh_guitar_buttons;
    case RockBandGuitar: return rb_guitar_buttons;
    case LiveGuitar: return ghl_guitar_buttons;
    case GuitarHeroDrums: return gh_drum_buttons;
    case RockBandDrums: return rb_drum_buttons;
    case DjHeroTurntable: return turntable_buttons;
    case ProGuitarMustang:
    case ProGuitarSquire:
    case ProKeys: return pro_buttons;
    case StageKit: return stage_kit_buttons;
    // Gamepad, Dancepad, Taiko and the portals all use the gamepad report
    default: return gamepad_buttons;
    }
}
} // namespace

bool santroller_v1_tick_digital(const uint8_t *report, SubType subtype, proto_Output &type)
{
    uint8_t hat = report[3] & 0x0F;
    bool up = (hat == 0 || hat == 1 || hat == 7);
    bool down = (hat == 3 || hat == 4 || hat == 5);
    int value = type.mapping.gamepadButton;

    switch (type.which_mapping)
    {
    case proto_Output_gamepadButton_tag:
    {
        // Rock Band kits press the face buttons and d-pad when hitting pads and cymbals,
        // only treat them as navigation when no hit is flagged (byte 1: pad flag, cymbal flag)
        if (subtype == RockBandDrums && (report[1] & 0x30) &&
            (value <= Gamepad_Y || value >= Gamepad_DpadUp))
            return false;
        switch (value)
        {
        case Gamepad_DpadUp: return up;
        case Gamepad_DpadDown: return down;
        case Gamepad_DpadLeft: return hat == 5 || hat == 6 || hat == 7;
        case Gamepad_DpadRight: return hat == 1 || hat == 2 || hat == 3;
        }
        if (value < 1 || value > GAMEPAD_BUTTON_COUNT)
            return false;
        bool pressed = button(report, gamepad_button_table(subtype)[value - 1]);
        if (subtype == RockBandGuitar)
            pressed |= button(report, rb_guitar_solo_buttons[value - 1]);
        return pressed;
    }
    case proto_Output_ghButton_tag:
        if (subtype != GuitarHeroGuitar)
            return false;
        if (value >= GuitarHeroGuitar_TapGreen && value <= GuitarHeroGuitar_TapOrange)
        {
            // Santroller 1 sends the slider as the PS3 GH5 slider value
            return decode_gh5_slider(report[5]) & (1 << (value - GuitarHeroGuitar_TapGreen));
        }
        if (value == GuitarHeroGuitar_Pedal)
            return button(report, 5);
        // green, red, yellow, blue, orange are bits 0 - 4
        return value >= GuitarHeroGuitar_Green && value <= GuitarHeroGuitar_Orange && button(report, value - 1);
    case proto_Output_rbButton_tag:
        // green, red, yellow, blue, orange, then the solo frets are bits 0 - 9
        return subtype == RockBandGuitar && value >= RockBandGuitar_Green &&
               value <= RockBandGuitar_SoloOrange && button(report, value - 1);
    case proto_Output_ghlButton_tag:
        if (subtype != LiveGuitar)
            return false;
        if (value == GuitarHeroLiveGuitar_StrumUp)
            return up;
        if (value == GuitarHeroLiveGuitar_StrumDown)
            return down;
        return button(report, lookup(ghl_frets, value));
    case proto_Output_djhButton_tag:
        // right green, red, blue, left green, red, blue are bits 7 - 12
        return subtype == DjHeroTurntable && value >= DJHTurntable_LeftBlue &&
               value <= DJHTurntable_RightGreen && button(report, 13 - value);
    case proto_Output_rbDrumButton_tag:
        // kick 1 and 2 are bits 6 and 7
        return subtype == RockBandDrums && value >= RockBandDrums_Kick1Pedal &&
               value <= RockBandDrums_Kick2Pedal && button(report, value - RockBandDrums_Kick1Pedal + 6);
    case proto_Output_proButton_tag:
        if (subtype != ProGuitarMustang && subtype != ProGuitarSquire)
            return false;
        // green, red, yellow, blue, orange, pedal are bits 7 - 12. There's no solo flag, so the
        // solo frets never read as held
        if (value == ProGuitar_Pedal)
            return button(report, 12);
        return value >= ProGuitar_Green && value <= ProGuitar_Orange && button(report, value - ProGuitar_Green + 7);
    case proto_Output_proKeyboardButton_tag:
        return subtype == ProKeys && value == ProKeyboardOverdrive && button(report, 7);
    case proto_Output_proKeySingle_tag:
    {
        if (subtype != ProKeys)
            return false;
        auto data = (const Santroller1ProKeys_Data_t *)report;
        return pro_keyboard_key_pressed(data->key1, data->key2, data->key3, data->velocities, type.mapping.proKeySingle);
    }
    case proto_Output_proKeyMultiple_tag:
    {
        if (subtype != ProKeys)
            return false;
        auto data = (const Santroller1ProKeys_Data_t *)report;
        return pro_keyboard_any_key_pressed(data->key1, data->key2, data->key3, data->velocities, type.mapping.proKeyMultiple);
    }
    case proto_Output_ghDrumAxis_tag:
    case proto_Output_rbDrumAxis_tag:
        // a pad is hit while it has a velocity
        return santroller_v1_tick_analog(report, subtype, type) != 0;
    default:
        return false;
    }
}

uint16_t santroller_v1_tick_analog(const uint8_t *report, SubType subtype, proto_Output &type)
{
    int value = type.mapping.gamepadAxis;
    switch (type.which_mapping)
    {
    case proto_Output_gamepadAxis_tag:
        switch (subtype)
        {
        case GuitarHeroGuitar:
        case RockBandGuitar:
        case LiveGuitar:
        case GuitarHeroDrums:
        case RockBandDrums:
        case DjHeroTurntable:
        case ProGuitarMustang:
        case ProGuitarSquire:
        case ProKeys:
            return 0;
        default:
        {
            // Gamepad, Dancepad, Taiko, the portals and the stage kit all have the gamepad's axes
            uint16_t result = axis(report, gamepad_axes, value);
            // HID y is down at the max
            if (value == Gamepad_LeftStickY || value == Gamepad_RightStickY)
                result = ~result;
            return result;
        }
        }
    case proto_Output_ghAxis_tag:
        return subtype == GuitarHeroGuitar ? axis(report, gh_guitar_axes, value) : 0;
    case proto_Output_rbAxis_tag:
        return subtype == RockBandGuitar ? axis(report, rb_guitar_axes, value) : 0;
    case proto_Output_ghlAxis_tag:
        return subtype == LiveGuitar ? axis(report, ghl_guitar_axes, value) : 0;
    case proto_Output_ghDrumAxis_tag:
        return subtype == GuitarHeroDrums ? axis(report, gh_drum_axes, value) : 0;
    case proto_Output_rbDrumAxis_tag:
        return subtype == RockBandDrums ? axis(report, rb_drum_axes, value) : 0;
    case proto_Output_djhAxis_tag:
        return subtype == DjHeroTurntable ? axis(report, turntable_axes, value) : 0;
    case proto_Output_proAxis_tag:
        if (subtype != ProGuitarMustang && subtype != ProGuitarSquire)
            return 0;
        // byte 4: tilt, 5 - 10: frets low E to high E, 11 - 16: their velocities. Same
        // scaling as the PS3 pro guitar, which Santroller 1 copies these from
        if (value >= ProGuitar_LowEFret && value <= ProGuitar_HighEFret)
            return report[4 + value] << 11;
        if (value >= ProGuitar_LowEFretVelocity && value <= ProGuitar_HighEFretVelocity)
            return report[4 + value] << 9;
        if (value == ProGuitar_Tilt)
            return report[4] << 9;
        return 0;
    case proto_Output_proKeyboardAxis_tag:
    {
        if (subtype != ProKeys)
            return 0;
        auto data = (const Santroller1ProKeys_Data_t *)report;
        if (value == ProKeyboardPedal)
            return (0x7f - (data->pedalAnalog & 0x7f)) << 9;
        if (value == ProKeyboardTouchPad)
            return (data->touchPad & 0x7f) << 9;
        return 0;
    }
    default:
        return 0;
    }
}

uint16_t santroller_v1_tick_button_pressure(const uint8_t *report, SubType subtype, proto_Output &type)
{
    if (!santroller_v1_tick_digital(report, subtype, type))
        return 0;

    if (subtype == ProKeys)
    {
        auto data = (const Santroller1ProKeys_Data_t *)report;
        if (type.which_mapping == proto_Output_proKeySingle_tag)
            return pro_keyboard_key_pressure(data->key1, data->key2, data->key3,
                                             data->velocities, type.mapping.proKeySingle);
        if (type.which_mapping == proto_Output_proKeyMultiple_tag)
            return pro_keyboard_key_range_pressure(data->key1, data->key2, data->key3,
                                                   data->velocities, type.mapping.proKeyMultiple);
    }

    return UINT16_MAX;
}
