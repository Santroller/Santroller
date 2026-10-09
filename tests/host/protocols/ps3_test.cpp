#include <gtest/gtest.h>
#include <cstddef>
#include "protocols/ps3.hpp"
#include "protocols/proto_output.hpp"

TEST(Ps3, LedBitmapForPlayers)
{
    // DS3 LED bitmap: LED_1 = 0x02, LED_2 = 0x04, ... (bit 0 is unused)
    EXPECT_EQ(ps3_leds_bitmap_for_player(1), 0x02);
    EXPECT_EQ(ps3_leds_bitmap_for_player(2), 0x04);
    EXPECT_EQ(ps3_leds_bitmap_for_player(3), 0x08);
    EXPECT_EQ(ps3_leds_bitmap_for_player(4), 0x10);
}

TEST(Ps3, UnassignedOrOutOfRangePlayersShowLed1)
{
    EXPECT_EQ(ps3_leds_bitmap_for_player(0), 0x02);
    EXPECT_EQ(ps3_leds_bitmap_for_player(5), 0x02);
    EXPECT_EQ(ps3_leds_bitmap_for_player(7), 0x02);
    EXPECT_EQ(ps3_leds_bitmap_for_player(0xFF), 0x02);
}

TEST(Ps3, OutputReportDefaultsMatchTheSixaxisDefaults)
{
    // Linux hid-sony's default sixaxis output report, minus the report id, and with the first
    // byte 0 like a PS3 sends: rumble off with 0xff durations, no LEDs selected, each LED solid on
    const uint8_t expected[35] = {
        0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x27, 0x10, 0x00, 0x32,
        0xff, 0x27, 0x10, 0x00, 0x32,
        0xff, 0x27, 0x10, 0x00, 0x32,
        0xff, 0x27, 0x10, 0x00, 0x32,
        0x00, 0x00, 0x00, 0x00, 0x00};
    ps3_output_report report;
    memset(&report, 0xAA, sizeof(report));
    ps3_output_report_init(&report);
    ASSERT_EQ(sizeof(report), sizeof(expected));
    const uint8_t *bytes = raw_bytes(report);
    for (size_t i = 0; i < sizeof(expected); i++)
        EXPECT_EQ(bytes[i], expected[i]) << "byte " << i;
}

TEST(Ps3, OutputReportFieldOffsets)
{
    // Linux hid-sony sixaxis_output_report without the report id
    EXPECT_EQ(offsetof(ps3_output_report, rumble), 0u);
    EXPECT_EQ(offsetof(ps3_rumble_t, right_duration), 1u);
    EXPECT_EQ(offsetof(ps3_rumble_t, right_motor_on), 2u);
    EXPECT_EQ(offsetof(ps3_rumble_t, left_duration), 3u);
    EXPECT_EQ(offsetof(ps3_rumble_t, left_motor_force), 4u);
    EXPECT_EQ(offsetof(ps3_output_report, leds_bitmap), 9u);
    EXPECT_EQ(offsetof(ps3_output_report, led), 10u);
    EXPECT_EQ(sizeof(ps3_led_t), 5u);
}

TEST(Ps3, Ds3InputReportLayout)
{
    // 49 byte DS3 input report including report id 1, offsets from Linux hid-sony / the sixaxis mapping
    EXPECT_EQ(sizeof(PS3Gamepad_Data_t), 49u);
    EXPECT_EQ(offsetof(PS3Gamepad_Data_t, leftStickX), 6u);
    EXPECT_EQ(offsetof(PS3Gamepad_Data_t, rightStickY), 9u);
    EXPECT_EQ(offsetof(PS3Gamepad_Data_t, pressureDpadUp), 14u);
    EXPECT_EQ(offsetof(PS3Gamepad_Data_t, leftTrigger), 18u);
    EXPECT_EQ(offsetof(PS3Gamepad_Data_t, pressureSquare), 25u);
    EXPECT_EQ(offsetof(PS3Gamepad_Data_t, battery_status), 30u);
    EXPECT_EQ(offsetof(PS3Gamepad_Data_t, accelX), 41u); // SIXAXIS_INPUT_REPORT_ACC_X_OFFSET
    EXPECT_EQ(offsetof(PS3Gamepad_Data_t, gyro), 47u);
}

TEST(Ps3, Ds3ButtonBits)
{
    // byte 2: select L3 R3 start up right down left; byte 3: L2 R2 L1 R1 triangle circle cross square; byte 4: PS
    EXPECT_BIT_AT(PS3Gamepad_Data_t, back, 2, 0x01);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, leftThumbClick, 2, 0x02);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, rightThumbClick, 2, 0x04);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, start, 2, 0x08);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, dpadUp, 2, 0x10);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, dpadRight, 2, 0x20);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, dpadDown, 2, 0x40);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, dpadLeft, 2, 0x80);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, l2, 3, 0x01);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, r2, 3, 0x02);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, leftShoulder, 3, 0x04);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, rightShoulder, 3, 0x08);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, y, 3, 0x10);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, b, 3, 0x20);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, a, 3, 0x40);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, x, 3, 0x80);
    EXPECT_BIT_AT(PS3Gamepad_Data_t, guide, 4, 0x01);
}

TEST(Ps3, HidReportsAreTwentySevenBytes)
{
    // PS3 HID gamepads and instruments send a 27 byte report with no report id: 13 buttons + hat,
    // 4 sticks, 12 pressure axes, 4 x 16 bit motion axes. Instrument reports go out in the same buffer
    // with sizeof(PS3Dpad_Data_t).
    EXPECT_EQ(sizeof(PS3Dpad_Data_t), 27u);
    EXPECT_EQ(offsetof(PS3Dpad_Data_t, leftStickX), 3u);
    EXPECT_EQ(offsetof(PS3Dpad_Data_t, accelX), 19u);
    EXPECT_EQ(sizeof(PS3ThirdPartyGamepad_Data_t), 27u);
    EXPECT_EQ(sizeof(PS3RockBandGuitar_Data_t), 27u);
    EXPECT_EQ(sizeof(PS3GuitarHeroGuitar_Data_t), 27u);
    EXPECT_EQ(sizeof(PS3RockBandDrums_Data_t), 27u);
    EXPECT_EQ(sizeof(PS3GuitarHeroDrums_Data_t), 27u);
    EXPECT_EQ(sizeof(PS3DJHTurntable_Data_t), 27u);
    EXPECT_EQ(sizeof(PS3GHLGuitar_Data_t), 27u);
    EXPECT_EQ(sizeof(PS3RockBandProGuitar_Data_t), 27u);
    EXPECT_EQ(sizeof(PS3RockBandProKeyboard_Data_t), 27u);
    EXPECT_EQ(sizeof(PS2GuitarOnPS3_Data_t), 27u);
}

TEST(Ps3, HidGamepadButtonBits)
{
    // Standard PS3 HID gamepad order: square cross circle triangle L1 R1 L2 R2, select start L3 R3 PS
    EXPECT_BIT_AT(PS3Dpad_Data_t, x, 0, 0x01);
    EXPECT_BIT_AT(PS3Dpad_Data_t, a, 0, 0x02);
    EXPECT_BIT_AT(PS3Dpad_Data_t, b, 0, 0x04);
    EXPECT_BIT_AT(PS3Dpad_Data_t, y, 0, 0x08);
    EXPECT_BIT_AT(PS3Dpad_Data_t, leftShoulder, 0, 0x10);
    EXPECT_BIT_AT(PS3Dpad_Data_t, rightShoulder, 0, 0x20);
    EXPECT_BIT_AT(PS3Dpad_Data_t, l2, 0, 0x40);
    EXPECT_BIT_AT(PS3Dpad_Data_t, r2, 0, 0x80);
    EXPECT_BIT_AT(PS3Dpad_Data_t, back, 1, 0x01);
    EXPECT_BIT_AT(PS3Dpad_Data_t, start, 1, 0x02);
    EXPECT_BIT_AT(PS3Dpad_Data_t, leftThumbClick, 1, 0x04);
    EXPECT_BIT_AT(PS3Dpad_Data_t, rightThumbClick, 1, 0x08);
    EXPECT_BIT_AT(PS3Dpad_Data_t, guide, 1, 0x10);
    EXPECT_BIT_AT(PS3Dpad_Data_t, dpad, 2, 0x01);
}
