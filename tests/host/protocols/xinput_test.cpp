#include <gtest/gtest.h>
#include <cstddef>
#include "protocols/xinput.hpp"
#include "protocols/proto_output.hpp"

// XInput device subtypes, from Microsoft's XINPUT_DEVSUBTYPE_* values and the instrument subtypes
TEST(XInput, SubtypeValues)
{
    EXPECT_EQ(XINPUT_GAMEPAD, 0x01);
    EXPECT_EQ(XINPUT_WHEEL, 0x02);
    EXPECT_EQ(XINPUT_ARCADE_STICK, 0x03);
    EXPECT_EQ(XINPUT_FLIGHT_STICK, 0x04);
    EXPECT_EQ(XINPUT_DANCE_PAD, 0x05);
    EXPECT_EQ(XINPUT_GUITAR, 0x06);
    EXPECT_EQ(XINPUT_GUITAR_ALTERNATE, 0x07);
    EXPECT_EQ(XINPUT_DRUMS, 0x08);
    EXPECT_EQ(XINPUT_GUITAR_BASS, 0x0B);
    EXPECT_EQ(XINPUT_ARCADE_PAD, 0x13);
}

TEST(XInput, SubtypesRoundTrip)
{
    for (SubType t : {Gamepad, Wheel, FightStick, FlightStick, Dancepad, RockBandGuitar, GuitarHeroGuitar, RockBandDrums,
                      StageKit, ProKeys, DjHeroTurntable, ProGuitarSquire, DisneyInfinity, Skylanders})
        EXPECT_EQ(get_subtype_from_xinput(get_xinput_subtype(t)), t) << t;
}

TEST(XInput, SubtypesXInputCantTellApart)
{
    // XInput only has one drum and one pro guitar subtype, and Guitar Hero drums use the drum subtype
    EXPECT_EQ(get_xinput_subtype(GuitarHeroDrums), XINPUT_DRUMS);
    EXPECT_EQ(get_xinput_subtype(ProGuitarMustang), XINPUT_PRO_GUITAR);
    EXPECT_EQ(get_xinput_subtype(LegoDimensions), XINPUT_DISNEY_INFINITY_AND_LEGO_DIMENSIONS);
    EXPECT_EQ(get_subtype_from_xinput(XINPUT_GUITAR_BASS), RockBandGuitar);
    EXPECT_EQ(get_subtype_from_xinput(XINPUT_ARCADE_PAD), Gamepad);
}

TEST(XInput, GamepadLikeSubtypesAreGamepads)
{
    for (SubType t : {PopNMusic, BeatMania, ProjectDiva, DJMax, Taiko, KeyboardMouse, Midi, Unknown})
        EXPECT_EQ(get_xinput_subtype(t), XINPUT_GAMEPAD) << t;
}

TEST(XInput, UnknownXInputSubtypesAreGamepads)
{
    for (uint8_t sub : {0x00, 0x0A, 0x0C, 0x10, 0x14, 0xFF})
        EXPECT_EQ(get_subtype_from_xinput(sub), Gamepad) << int(sub);
}

// Wire formats, from Linux xpad and the XInput descriptor layouts

TEST(XInput, GamepadReportLayout)
{
    // 20 byte report: type 0, length 0x14, 16 button bits, 8 bit triggers, 16 bit sticks
    EXPECT_EQ(sizeof(XInputGamepad_Data_t), 20u);
    EXPECT_EQ(offsetof(XInputGamepad_Data_t, leftTrigger), 4u);
    EXPECT_EQ(offsetof(XInputGamepad_Data_t, rightTrigger), 5u);
    EXPECT_EQ(offsetof(XInputGamepad_Data_t, leftStickX), 6u);
    EXPECT_EQ(offsetof(XInputGamepad_Data_t, leftStickY), 8u);
    EXPECT_EQ(offsetof(XInputGamepad_Data_t, rightStickX), 10u);
    EXPECT_EQ(offsetof(XInputGamepad_Data_t, rightStickY), 12u);
}

TEST(XInput, GamepadButtonBits)
{
    // XINPUT_GAMEPAD_* button flags, little endian at byte 2
    EXPECT_BIT_AT(XInputGamepad_Data_t, dpadUp, 2, 0x01);
    EXPECT_BIT_AT(XInputGamepad_Data_t, dpadDown, 2, 0x02);
    EXPECT_BIT_AT(XInputGamepad_Data_t, dpadLeft, 2, 0x04);
    EXPECT_BIT_AT(XInputGamepad_Data_t, dpadRight, 2, 0x08);
    EXPECT_BIT_AT(XInputGamepad_Data_t, start, 2, 0x10);
    EXPECT_BIT_AT(XInputGamepad_Data_t, back, 2, 0x20);
    EXPECT_BIT_AT(XInputGamepad_Data_t, leftThumbClick, 2, 0x40);
    EXPECT_BIT_AT(XInputGamepad_Data_t, rightThumbClick, 2, 0x80);
    EXPECT_BIT_AT(XInputGamepad_Data_t, leftShoulder, 3, 0x01);
    EXPECT_BIT_AT(XInputGamepad_Data_t, rightShoulder, 3, 0x02);
    EXPECT_BIT_AT(XInputGamepad_Data_t, guide, 3, 0x04);
    EXPECT_BIT_AT(XInputGamepad_Data_t, a, 3, 0x10);
    EXPECT_BIT_AT(XInputGamepad_Data_t, b, 3, 0x20);
    EXPECT_BIT_AT(XInputGamepad_Data_t, x, 3, 0x40);
    EXPECT_BIT_AT(XInputGamepad_Data_t, y, 3, 0x80);
}

TEST(XInput, InstrumentReportsAreGamepadSized)
{
    // instruments send the same 20 byte report (the firmware sends sizeof(XInputGamepad_Data_t))
    EXPECT_EQ(sizeof(XInputRockBandDrums_Data_t), 20u);
    EXPECT_EQ(sizeof(XInputGuitarHeroDrums_Data_t), 20u);
    EXPECT_EQ(sizeof(XInputGuitarHeroGuitar_Data_t), 20u);
    EXPECT_EQ(sizeof(XInputRockBandGuitar_Data_t), 20u);
    EXPECT_EQ(sizeof(XInputRockBandProGuitar_Data_t), 20u);
    EXPECT_EQ(sizeof(XInputRockBandKeyboard_Data_t), 20u);
    EXPECT_EQ(sizeof(XInputGHLGuitar_Data_t), 20u);
    EXPECT_EQ(sizeof(XInputDJHTurntable_Data_t), 20u);
}

TEST(XInput, InstrumentAxesShareTheGamepadAxes)
{
    // Instrument axes are read through the gamepad axes by XInput games, so they must line up
    EXPECT_EQ(offsetof(XInputRockBandDrums_Data_t, redVelocity), offsetof(XInputGamepad_Data_t, leftStickX));
    EXPECT_EQ(offsetof(XInputRockBandDrums_Data_t, greenVelocity), offsetof(XInputGamepad_Data_t, rightStickY));
    EXPECT_EQ(offsetof(XInputGuitarHeroGuitar_Data_t, whammy), offsetof(XInputGamepad_Data_t, rightStickX));
    EXPECT_EQ(offsetof(XInputGuitarHeroGuitar_Data_t, tilt), offsetof(XInputGamepad_Data_t, rightStickY));
    EXPECT_EQ(offsetof(XInputRockBandGuitar_Data_t, whammy), offsetof(XInputGamepad_Data_t, rightStickX));
    EXPECT_EQ(offsetof(XInputRockBandGuitar_Data_t, tilt), offsetof(XInputGamepad_Data_t, rightStickY));
    EXPECT_EQ(offsetof(XInputGHLGuitar_Data_t, strumBar), offsetof(XInputGamepad_Data_t, leftStickY));
    EXPECT_EQ(offsetof(XInputDJHTurntable_Data_t, leftTableVelocity), offsetof(XInputGamepad_Data_t, leftStickX));
    EXPECT_EQ(offsetof(XInputDJHTurntable_Data_t, crossfader), offsetof(XInputGamepad_Data_t, rightStickY));
}

TEST(XInput, OutputReports)
{
    // LED: 01 03 <pattern>; rumble: 00 08 00 <strong> <weak> 00 00 00
    EXPECT_EQ(sizeof(XInputLEDReport_t), 3u);
    EXPECT_EQ(sizeof(XInputRumbleReport_t), 8u);
    EXPECT_EQ(offsetof(XInputRumbleReport_t, leftRumble), 3u);
    EXPECT_EQ(offsetof(XInputRumbleReport_t, rightRumble), 4u);
    EXPECT_EQ(XBOX_LED_ID, 0x01);
    EXPECT_EQ(XBOX_RUMBLE_ID, 0x00);
    EXPECT_EQ(LED_ONE, 6);
    EXPECT_EQ(LED_FOUR, 9);
}

TEST(XInput, CapabilityReports)
{
    // GET_CAPABILITIES responses: 20 byte input capabilities and 8 byte vibration capabilities
    EXPECT_EQ(sizeof(XInputInputCapabilities_t), 20u);
    EXPECT_EQ(sizeof(XInputVibrationCapabilities_t), 8u);
    EXPECT_EQ(offsetof(XInputInputCapabilities_t, leftThumbX), 6u);
    EXPECT_EQ(offsetof(XInputVibrationCapabilities_t, left_motor), 3u);
}

TEST(XInput, Descriptors)
{
    // The 0x21 descriptor of a wired gamepad is 17 bytes:
    // 11 21 00 01 <subtype> 25 81 14 00 00 00 00 13 01 08 00 00
    EXPECT_EQ(sizeof(XBOX_ID_DESCRIPTOR), 0x11u);
    EXPECT_EQ(offsetof(XBOX_ID_DESCRIPTOR, subtype), 4u);
    EXPECT_EQ(offsetof(XBOX_ID_DESCRIPTOR, bEndpointAddressIn), 6u);
    EXPECT_EQ(offsetof(XBOX_ID_DESCRIPTOR, bMaxDataSizeIn), 7u);
    EXPECT_EQ(offsetof(XBOX_ID_DESCRIPTOR, bEndpointAddressOut), 13u);
    EXPECT_EQ(offsetof(XBOX_ID_DESCRIPTOR, bMaxDataSizeOut), 14u);
    // security interface descriptor: 06 41 00 01 01 03
    EXPECT_EQ(sizeof(XBOX_SECURITY_DESCRIPTOR), 6u);
}
