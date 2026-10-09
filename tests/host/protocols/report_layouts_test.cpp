#include <gtest/gtest.h>
#include <cstddef>
#include "protocols/controller_reports.hpp"
#include "protocols/og_xbox.hpp"
#include "protocols/pdloader.hpp"
#include "protocols/ps2.hpp"
#include "protocols/ps4.hpp"
#include "protocols/ps5.hpp"
#include "protocols/spice2x.hpp"
#include "protocols/switch.hpp"
#include "protocols/wii.hpp"
#include "protocols/xbox_gip.h"
#include "protocols/xbox_one.hpp"
#include "protocols/proto_output.hpp"

// Sizes and offsets of reports that go over the wire. Expected values come from the protocols
// (Linux hid-playstation / hid-sony / xpad / hid-nintendo, SDL, wiibrew) or from the lengths the
// firmware itself declares for them (descriptors, buffer sizes).

// ---------------------------------------------------------------------------------------------
// DualShock 4

TEST(Ps4Layout, InputReportIsSixtyFourBytes)
{
    // USB input report 0x01; the PS4 emulation sends sizeof(PS4Dpad_Data_t)
    EXPECT_EQ(sizeof(PS4Dpad_Data_t), 64u);
    EXPECT_EQ(sizeof(PS4Gamepad_Data_t), 64u);
    EXPECT_EQ(sizeof(PS4GHLGuitar_Data_t), 64u);
}

TEST(Ps4Layout, InputReportOffsets)
{
    // hid-playstation dualshock4_input_report_usb, plus one for the report id
    EXPECT_EQ(offsetof(PS4Dpad_Data_t, leftStickX), 1u);
    EXPECT_EQ(offsetof(PS4Dpad_Data_t, rightStickY), 4u);
    EXPECT_BYTE_AT(PS4Dpad_Data_t, leftTrigger, 8);
    EXPECT_BYTE_AT(PS4Dpad_Data_t, rightTrigger, 9);
    // num_touch_reports at 33, then the first touch report's timestamp, then its two points
    EXPECT_EQ(offsetof(PS4Dpad_Data_t, touchpadData), 35u);
}

TEST(Ps4Layout, ButtonBits)
{
    // hat in the low nibble of byte 5, then square cross circle triangle; L1 R1 L2 R2 share options L3 R3;
    // PS, touchpad click, then the 6 bit counter
    EXPECT_BIT_AT(PS4Dpad_Data_t, dpad, 5, 0x01);
    EXPECT_BIT_AT(PS4Dpad_Data_t, x, 5, 0x10);
    EXPECT_BIT_AT(PS4Dpad_Data_t, a, 5, 0x20);
    EXPECT_BIT_AT(PS4Dpad_Data_t, b, 5, 0x40);
    EXPECT_BIT_AT(PS4Dpad_Data_t, y, 5, 0x80);
    EXPECT_BIT_AT(PS4Dpad_Data_t, leftShoulder, 6, 0x01);
    EXPECT_BIT_AT(PS4Dpad_Data_t, rightShoulder, 6, 0x02);
    EXPECT_BIT_AT(PS4Dpad_Data_t, l2, 6, 0x04);
    EXPECT_BIT_AT(PS4Dpad_Data_t, r2, 6, 0x08);
    EXPECT_BIT_AT(PS4Dpad_Data_t, back, 6, 0x10);
    EXPECT_BIT_AT(PS4Dpad_Data_t, start, 6, 0x20);
    EXPECT_BIT_AT(PS4Dpad_Data_t, leftThumbClick, 6, 0x40);
    EXPECT_BIT_AT(PS4Dpad_Data_t, rightThumbClick, 6, 0x80);
    EXPECT_BIT_AT(PS4Dpad_Data_t, guide, 7, 0x01);
    EXPECT_BIT_AT(PS4Dpad_Data_t, capture, 7, 0x02);
    EXPECT_BIT_AT(PS4Dpad_Data_t, reportCounter, 7, 0x04);
}

TEST(Ps4Layout, RockBandInstruments)
{
    // same header as the DS4 report; the power level is the DS4 status byte (status[0] at 29 in
    // hid-playstation, 30 with the report id), which the PS4 emulation writes as PS4_STATUS_BYTE
    EXPECT_BYTE_AT(PS4RockBandGuitar_Data_t, leftTrigger, 8);
    EXPECT_BYTE_AT(PS4RockBandDrums_Data_t, leftTrigger, 8);
    EXPECT_BIT_AT(PS4RockBandGuitar_Data_t, powerLevel, 30, 0x01);
    EXPECT_BIT_AT(PS4RockBandDrums_Data_t, powerLevel, 30, 0x01);
}

// The Rock Band 4 PS4 instruments use the 64 byte DS4 USB report: they're written into the 64 byte HID
// buffer and sent with sizeof(PS4Dpad_Data_t)
TEST(Ps4Layout, RockBandInstrumentsAreTheUsbReport)
{
    EXPECT_EQ(sizeof(PS4RockBandGuitar_Data_t), 64u);
    EXPECT_EQ(sizeof(PS4RockBandDrums_Data_t), 64u);
}

TEST(Ps4Layout, OutputReport)
{
    // hid-playstation dualshock4_output_report_usb: report 0x05, 32 bytes
    EXPECT_EQ(sizeof(ps4_output_report), 32u);
    EXPECT_EQ(offsetof(ps4_output_report, motor_right), 4u);
    EXPECT_EQ(offsetof(ps4_output_report, motor_left), 5u);
    EXPECT_EQ(offsetof(ps4_output_report, lightbar_red), 6u);
    EXPECT_EQ(offsetof(ps4_output_report, lightbar_green), 7u);
    EXPECT_EQ(offsetof(ps4_output_report, lightbar_blue), 8u);
    EXPECT_EQ(offsetof(ps4_output_report, lightbar_blink_off), 10u);
}

TEST(Ps4Layout, AuthReports)
{
    // byte ranges given in the header comments
    EXPECT_EQ(sizeof(AuthReport), 64u);
    EXPECT_EQ(offsetof(AuthReport, data), 4u);
    EXPECT_EQ(sizeof(AuthReport::data), (size_t)PAYLOAD_MAX);
    EXPECT_EQ(offsetof(AuthReport, crc32), 60u);
    EXPECT_EQ(sizeof(AuthStatusReport), 16u);
    EXPECT_EQ(offsetof(AuthStatusReport, crc32), 12u);
}

// ---------------------------------------------------------------------------------------------
// DualSense

TEST(Ps5Layout, InputReportIsSixtyFourBytes)
{
    // DS_INPUT_REPORT_USB_SIZE; the PS5 emulation sends sizeof(PS5Dpad_Data_t)
    EXPECT_EQ(sizeof(PS5Dpad_Data_t), 64u);
}

TEST(Ps5Layout, InputReportOffsets)
{
    // hid-playstation dualsense_input_report, plus one for the report id
    EXPECT_EQ(offsetof(PS5Dpad_Data_t, leftStickX), 1u);
    EXPECT_EQ(offsetof(PS5Dpad_Data_t, leftTrigger), 5u);
    EXPECT_EQ(offsetof(PS5Dpad_Data_t, rightTrigger), 6u);
    EXPECT_EQ(offsetof(PS5Dpad_Data_t, gyroscope), 16u);
    EXPECT_EQ(offsetof(PS5Dpad_Data_t, accelerometer), 22u);
    EXPECT_EQ(sizeof(TouchpadData), 8u);
}

// hid-playstation's dualsense_input_report has a 32 bit sensor timestamp at 28 (with the report id)
// followed by a reserved byte, so the two touch points start at 33
TEST(Ps5Layout, TouchpadFollowsTheTimestampAndReservedByte)
{
    EXPECT_EQ(offsetof(PS5Dpad_Data_t, touchpad_data), 33u);
}

TEST(Ps5Layout, ButtonBits)
{
    EXPECT_BIT_AT(PS5Dpad_Data_t, dpad, 8, 0x01);
    EXPECT_BIT_AT(PS5Dpad_Data_t, x, 8, 0x10);
    EXPECT_BIT_AT(PS5Dpad_Data_t, a, 8, 0x20);
    EXPECT_BIT_AT(PS5Dpad_Data_t, b, 8, 0x40);
    EXPECT_BIT_AT(PS5Dpad_Data_t, y, 8, 0x80);
    EXPECT_BIT_AT(PS5Dpad_Data_t, leftShoulder, 9, 0x01);
    EXPECT_BIT_AT(PS5Dpad_Data_t, rightThumbClick, 9, 0x80);
    EXPECT_BIT_AT(PS5Dpad_Data_t, guide, 10, 0x01);
    EXPECT_BIT_AT(PS5Dpad_Data_t, touchpad, 10, 0x02);
}

TEST(Ps5Layout, OutputReportFlagsAndMotors)
{
    // hid-playstation dualsense_output_report_common after the report id: valid_flag0 (bit 0 compatible
    // vibration), valid_flag1 (bit 2 lightbar, bit 4 player indicator), motor_right, motor_left
    EXPECT_BIT_AT(ps5_output_report, vibration_flag, 1, 0x01);
    EXPECT_BIT_AT(ps5_output_report, light_bar_flag, 2, 0x04);
    EXPECT_BIT_AT(ps5_output_report, player_indicator_flag, 2, 0x10);
    EXPECT_EQ(offsetof(ps5_output_report, motor_right), 3u);
    EXPECT_EQ(offsetof(ps5_output_report, motor_left), 4u);
}

// hid-playstation's dualsense_output_report_common is 47 bytes (static_assert there) with player_leds at
// 43 and lightbar_red / green / blue at 44 - 46, i.e. 44 and 45 - 47 after the report id; SDL's
// DS5EffectsState_t agrees. Ps5Host sends this to a real DualSense, and the PS5 emulation reads a
// console's output report with it.
TEST(Ps5Layout, OutputReportLedsMatchTheDualSense)
{
    EXPECT_EQ(sizeof(ps5_output_report), 48u);
    EXPECT_EQ(offsetof(ps5_output_report, player_indicator), 44u);
    EXPECT_EQ(offsetof(ps5_output_report, lightbar_red), 45u);
    EXPECT_EQ(offsetof(ps5_output_report, lightbar_green), 46u);
    EXPECT_EQ(offsetof(ps5_output_report, lightbar_blue), 47u);
}

// ---------------------------------------------------------------------------------------------
// PS2

TEST(Ps2Layout, DualShock2Response)
{
    // 2 button bytes, right then left stick, 12 pressure bytes
    EXPECT_EQ(sizeof(PS2Gamepad_Data_t), 18u);
    EXPECT_EQ(offsetof(PS2Gamepad_Data_t, rightStickX), 2u);
    EXPECT_EQ(offsetof(PS2Gamepad_Data_t, leftStickX), 4u);
    EXPECT_EQ(offsetof(PS2Gamepad_Data_t, pressureDpadRight), 6u);
    EXPECT_EQ(offsetof(PS2Gamepad_Data_t, rightTrigger), 17u);
    EXPECT_BIT_AT(PS2Gamepad_Data_t, back, 0, 0x01);
    EXPECT_BIT_AT(PS2Gamepad_Data_t, start, 0, 0x08);
    EXPECT_BIT_AT(PS2Gamepad_Data_t, dpadUp, 0, 0x10);
    EXPECT_BIT_AT(PS2Gamepad_Data_t, dpadLeft, 0, 0x80);
    EXPECT_BIT_AT(PS2Gamepad_Data_t, l2, 1, 0x01);
    EXPECT_BIT_AT(PS2Gamepad_Data_t, rightShoulder, 1, 0x08);
    EXPECT_BIT_AT(PS2Gamepad_Data_t, y, 1, 0x10);
    EXPECT_BIT_AT(PS2Gamepad_Data_t, x, 1, 0x80);
}

// ---------------------------------------------------------------------------------------------
// Original Xbox

TEST(OgXboxLayout, GamepadReport)
{
    // xpad: 20 bytes, buttons at 2, analog A B X Y black white at 4 - 9, triggers 10 / 11, sticks from 12
    EXPECT_EQ(sizeof(OGXboxGamepad_Data_t), 20u);
    EXPECT_EQ(offsetof(OGXboxGamepad_Data_t, a), 4u);
    EXPECT_EQ(offsetof(OGXboxGamepad_Data_t, y), 7u);
    EXPECT_EQ(offsetof(OGXboxGamepad_Data_t, rightShoulder), 8u); // black
    EXPECT_EQ(offsetof(OGXboxGamepad_Data_t, leftShoulder), 9u);  // white
    EXPECT_EQ(offsetof(OGXboxGamepad_Data_t, leftTrigger), 10u);
    EXPECT_EQ(offsetof(OGXboxGamepad_Data_t, rightTrigger), 11u);
    EXPECT_EQ(offsetof(OGXboxGamepad_Data_t, leftStickX), 12u);
    EXPECT_EQ(offsetof(OGXboxGamepad_Data_t, rightStickY), 18u);
    EXPECT_BIT_AT(OGXboxGamepad_Data_t, dpadUp, 2, 0x01);
    EXPECT_BIT_AT(OGXboxGamepad_Data_t, dpadRight, 2, 0x08);
    EXPECT_BIT_AT(OGXboxGamepad_Data_t, start, 2, 0x10);
    EXPECT_BIT_AT(OGXboxGamepad_Data_t, back, 2, 0x20);
    EXPECT_BIT_AT(OGXboxGamepad_Data_t, rightThumbClick, 2, 0x80);
}

TEST(OgXboxLayout, RumbleReport)
{
    // xpad: 00 06 00 <strong> 00 <weak>, i.e. two little endian 16 bit motor values
    EXPECT_EQ(sizeof(OGXboxOutput_Report_t), 6u);
    EXPECT_EQ(offsetof(OGXboxOutput_Report_t, left), 2u);
    EXPECT_EQ(offsetof(OGXboxOutput_Report_t, right), 4u);
}

TEST(OgXboxLayout, InstrumentReportsAreGamepadSized)
{
    // all sent with sizeof(OGXboxGamepad_Data_t) from the same buffer
    EXPECT_EQ(sizeof(OGXboxGamepadCapabilities_Data_t), 20u);
    EXPECT_EQ(sizeof(OGXboxRockBandKeyboard_Data_t), 20u);
    EXPECT_EQ(sizeof(OGXboxRockBandDrums_Data_t), 20u);
    EXPECT_EQ(sizeof(OGXboxGuitarHeroDrums_Data_t), 20u);
    EXPECT_EQ(sizeof(OGXboxGuitarHeroGuitar_Data_t), 20u);
    EXPECT_EQ(sizeof(OGXboxRockBandGuitar_Data_t), 20u);
    EXPECT_EQ(sizeof(OGXboxGHLGuitar_Data_t), 20u);
    EXPECT_EQ(sizeof(OGXboxDJHTurntable_Data_t), 20u);
    EXPECT_EQ(sizeof(OGXboxRockBandProGuitar_Data_t), 20u);
}

// ---------------------------------------------------------------------------------------------
// Xbox One (GIP)

TEST(XboxOneLayout, GamepadInput)
{
    // xpad: GIP input payload is 2 button bytes, 10 bit triggers as u16, then 16 bit sticks (14 bytes),
    // followed here by the console function map. Declared as 0x20 bytes in xb1_descriptor_gamepad.
    EXPECT_EQ(offsetof(XboxOneGamepad_Data_t, leftTrigger), 2u);
    EXPECT_EQ(offsetof(XboxOneGamepad_Data_t, rightTrigger), 4u);
    EXPECT_EQ(offsetof(XboxOneGamepad_Data_t, leftStickX), 6u);
    EXPECT_EQ(offsetof(XboxOneGamepad_Data_t, rightStickY), 12u);
    EXPECT_EQ(offsetof(XboxOneGamepad_Data_t, consoleFunctions), 14u);
    EXPECT_EQ(sizeof(XboxOneGamepad_Data_t), 0x20u);
}

TEST(XboxOneLayout, GamepadButtonBits)
{
    // xpad: byte 0 bit 2 menu, bit 3 view, then A B X Y; byte 1 up down left right LB RB LS RS
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, start, 0, 0x04);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, back, 0, 0x08);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, a, 0, 0x10);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, b, 0, 0x20);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, x, 0, 0x40);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, y, 0, 0x80);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, dpadUp, 1, 0x01);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, dpadDown, 1, 0x02);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, dpadLeft, 1, 0x04);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, dpadRight, 1, 0x08);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, leftShoulder, 1, 0x10);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, rightShoulder, 1, 0x20);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, leftThumbClick, 1, 0x40);
    EXPECT_BIT_AT(XboxOneGamepad_Data_t, rightThumbClick, 1, 0x80);
}

TEST(XboxOneLayout, InstrumentsMatchTheirDescriptors)
{
    // input (0x20) lengths declared in xb1_descriptor_guitar / _drum, output (0x22) in _ghl
    EXPECT_EQ(sizeof(XboxOneRockBandGuitar_Data_t), 0x20u);
    EXPECT_EQ(sizeof(XboxOneRockBandDrums_Data_t), 0x1Cu);
    EXPECT_LE(sizeof(XboxOneGHLGuitar_Data_t), 0x20u);
    EXPECT_EQ(sizeof(XboxOneGHLGuitar_Output_t), 0x08u);
    // all instruments share the gamepad's button header
    EXPECT_EQ(sizeof(XboxOneInputHeader_Data_t), 1u);
}

TEST(XboxOneLayout, RumbleFitsTheDeclaredLength)
{
    // 0x09 is declared with a 0x3C byte maximum in xb1_descriptor_gamepad; xpad sends 9 bytes:
    // sub command, motor mask, LT, RT, left, right, duration, delay, repeat
    EXPECT_EQ(sizeof(GipRumble_t), 9u);
    EXPECT_EQ(offsetof(GipRumble_t, leftMotor), 4u);
    EXPECT_EQ(offsetof(GipRumble_t, rightMotor), 5u);
}

TEST(XboxOneLayout, GipMessageIds)
{
    EXPECT_EQ(GIP_ACK_RESPONSE, 0x01);
    EXPECT_EQ(GIP_ANNOUNCE, 0x02);
    EXPECT_EQ(GIP_KEEPALIVE, 0x03);
    EXPECT_EQ(GIP_DEVICE_DESCRIPTOR, 0x04);
    EXPECT_EQ(GIP_POWER_MODE_DEVICE_CONFIG, 0x05);
    EXPECT_EQ(GIP_AUTH, 0x06);
    EXPECT_EQ(GIP_VIRTUAL_KEYCODE, 0x07);
    EXPECT_EQ(GIP_CMD_RUMBLE, 0x09);
    EXPECT_EQ(GIP_CMD_LED_ON, 0x0A);
    EXPECT_EQ(GIP_INPUT_REPORT, 0x20);
}

// ---------------------------------------------------------------------------------------------
// Switch

TEST(SwitchLayout, ProControllerFullReport)
{
    // report 0x30: id, timer, battery / connection, 3 button bytes, 6 stick bytes, vibrator, 36 IMU bytes
    EXPECT_EQ(sizeof(SwitchInputReport), 10u);
    EXPECT_EQ(offsetof(SwitchProGamepad_Data_t, inputs), 2u);
    EXPECT_EQ(offsetof(SwitchProGamepad_Data_t, rumbleReport), 12u);
    EXPECT_EQ(offsetof(SwitchProGamepad_Data_t, imuData), 13u);
    EXPECT_EQ(sizeof(SwitchProGamepad_Data_t), 64u);
}

TEST(SwitchLayout, ButtonBits)
{
    // hid-nintendo JC_BTN_*: right byte Y X B A SR SL R ZR, shared byte - + RS LS home capture,
    // left byte down up right left SR SL L ZL. Offsets here are within SwitchInputReport.
    EXPECT_BIT_AT(SwitchInputReport, y, 1, 0x01);
    EXPECT_BIT_AT(SwitchInputReport, x, 1, 0x02);
    EXPECT_BIT_AT(SwitchInputReport, b, 1, 0x04);
    EXPECT_BIT_AT(SwitchInputReport, a, 1, 0x08);
    EXPECT_BIT_AT(SwitchInputReport, buttonRightSR, 1, 0x10);
    EXPECT_BIT_AT(SwitchInputReport, buttonRightSL, 1, 0x20);
    EXPECT_BIT_AT(SwitchInputReport, rightShoulder, 1, 0x40);
    EXPECT_BIT_AT(SwitchInputReport, rightTrigger, 1, 0x80);
    EXPECT_BIT_AT(SwitchInputReport, back, 2, 0x01);
    EXPECT_BIT_AT(SwitchInputReport, start, 2, 0x02);
    EXPECT_BIT_AT(SwitchInputReport, guide, 2, 0x10);
    EXPECT_BIT_AT(SwitchInputReport, capture, 2, 0x20);
    EXPECT_BIT_AT(SwitchInputReport, dpadDown, 3, 0x01);
    EXPECT_BIT_AT(SwitchInputReport, dpadUp, 3, 0x02);
    EXPECT_BIT_AT(SwitchInputReport, dpadRight, 3, 0x04);
    EXPECT_BIT_AT(SwitchInputReport, dpadLeft, 3, 0x08);
    EXPECT_BIT_AT(SwitchInputReport, leftShoulder, 3, 0x40);
    EXPECT_BIT_AT(SwitchInputReport, leftTrigger, 3, 0x80);
}

// hid-nintendo has JC_BTN_RSTICK = BIT(10) and JC_BTN_LSTICK = BIT(11) (SDL: right stick 0x04, left
// stick 0x08 in the shared byte), matching SwitchHost's raw-byte Switch report path
TEST(SwitchLayout, StickClicksAreRightThenLeft)
{
    EXPECT_BIT_AT(SwitchInputReport, rightThumbClick, 2, 0x04);
    EXPECT_BIT_AT(SwitchInputReport, leftThumbClick, 2, 0x08);
}

// hid-nintendo: JC_BTN_SR_L is BIT(20) and JC_BTN_SL_L is BIT(21), the same SR-then-SL order as the
// right Joy-Con (and as buttonRightSR / buttonRightSL above)
TEST(SwitchLayout, LeftJoyConSideButtonsAreSrThenSl)
{
    EXPECT_BIT_AT(SwitchInputReport, buttonLeftSR, 3, 0x10);
    EXPECT_BIT_AT(SwitchInputReport, buttonLeftSL, 3, 0x20);
}

TEST(SwitchLayout, SticksAreTwelveBitPairs)
{
    // x = b0 | (b1 & 0xF) << 8, y = b1 >> 4 | b2 << 4
    SwitchInputReport r;
    memset(&r, 0, sizeof(r));
    r.leftStickX = 0xABC;
    r.leftStickY = 0x123;
    r.rightStickX = 0xFFF;
    r.rightStickY = 0x801;
    const uint8_t *b = raw_bytes(r);
    EXPECT_EQ(b[4], 0xBC);
    EXPECT_EQ(b[5], 0x3A);
    EXPECT_EQ(b[6], 0x12);
    EXPECT_EQ(b[7], 0xFF);
    EXPECT_EQ(b[8], 0x1F);
    EXPECT_EQ(b[9], 0x80);
}

// ---------------------------------------------------------------------------------------------
// Wii extensions (wiibrew)

TEST(WiiLayout, ClassicControllerFormats)
{
    EXPECT_EQ(sizeof(WiiClassicDataFormat1_t), 6u);
    EXPECT_EQ(sizeof(WiiClassicDataFormat2_t), 9u);
    EXPECT_EQ(sizeof(WiiClassicDataFormat3_t), 8u);
}

TEST(WiiLayout, ClassicFormat1Packing)
{
    // byte 0: RX<4:3> LX<5:0>; byte 1: RX<2:1> LY<5:0>; byte 2: RX<0> LT<4:3> RY<4:0>; byte 3: LT<2:0> RT<4:0>
    const uint8_t raw[6] = {0x80 | 0x15, 0x40 | 0x2A, 0x80 | 0x40 | 0x0B, 0xA0 | 0x11, 0x00, 0x00};
    WiiClassicDataFormat1_t r;
    memcpy(&r, raw, sizeof(r));
    EXPECT_EQ(r.leftStickX, 0x15);
    EXPECT_EQ(r.leftStickY, 0x2A);
    EXPECT_EQ(r.rightStickX43, 0x2);
    EXPECT_EQ(r.rightStickX21, 0x1);
    EXPECT_EQ(r.rightStickX0, 0x1);
    EXPECT_EQ(r.rightStickY, 0x0B);
    EXPECT_EQ(r.leftTrigger43, 0x2);
    EXPECT_EQ(r.leftTrigger20, 0x5);
    EXPECT_EQ(r.rightTrigger, 0x11);
}

TEST(WiiLayout, ClassicButtons)
{
    // byte 4: BDR BDD BLT B- BH B+ BRT 1 (bit 7 first), byte 5: BZL BB BY BA BX BZR BDL BDU
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, r2, 6, 0x02);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, start, 6, 0x04);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, guide, 6, 0x08);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, back, 6, 0x10);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, l2, 6, 0x20);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, dpadDown, 6, 0x40);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, dpadRight, 6, 0x80);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, dpadUp, 7, 0x01);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, dpadLeft, 7, 0x02);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, rightShoulder, 7, 0x04);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, x, 7, 0x08);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, a, 7, 0x10);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, y, 7, 0x20);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, b, 7, 0x40);
    EXPECT_BIT_AT(WiiClassicDataFormat3_t, leftShoulder, 7, 0x80);
    // the same two button bytes end formats 1 and 2
    EXPECT_EQ(offsetof(WiiClassicDataFormat1_t, buttonsLow), 4u);
    EXPECT_EQ(offsetof(WiiClassicDataFormat2_t, buttonsLow), 7u);
}

TEST(WiiLayout, ClassicFormat2LowBits)
{
    // byte 4: RY<1:0> LY<1:0> RX<1:0> LX<1:0>
    EXPECT_BIT_AT(WiiClassicDataFormat2_t, leftStickX10, 4, 0x01);
    EXPECT_BIT_AT(WiiClassicDataFormat2_t, rightStickX10, 4, 0x04);
    EXPECT_BIT_AT(WiiClassicDataFormat2_t, leftStickY10, 4, 0x10);
    EXPECT_BIT_AT(WiiClassicDataFormat2_t, rightStickY10, 4, 0x40);
}

TEST(WiiLayout, InstrumentExtensionsAreSixBytes)
{
    EXPECT_EQ(sizeof(WiiGuitarDataFormat3_t), 6u);
    EXPECT_EQ(sizeof(WiiDrumDataFormat3_t), 6u);
    EXPECT_EQ(sizeof(WiiTurntableDataFormat3_t), 6u);
}

// ---------------------------------------------------------------------------------------------
// Keyboards, Stadia, misc

TEST(ControllerReportsLayout, BootKeyboardIsEightBytes)
{
    // HID boot keyboard: modifiers, reserved, six keycodes
    EXPECT_EQ(sizeof(USB_6KRO_Boot_Data_t), 8u);
    EXPECT_EQ(offsetof(USB_6KRO_Boot_Data_t, KeyCode), 2u);
    EXPECT_EQ(sizeof(USB_6KRO_Data_t), 9u);
    EXPECT_BIT_AT(USB_6KRO_Boot_Data_t, leftCtrl, 0, 0x01);
    EXPECT_BIT_AT(USB_6KRO_Boot_Data_t, leftShift, 0, 0x02);
    EXPECT_BIT_AT(USB_6KRO_Boot_Data_t, rWin, 0, 0x80);
}

TEST(ControllerReportsLayout, Stadia)
{
    // SDL's HIDAPI Stadia driver: report 3, hat at 1, buttons at 2 / 3, sticks 4 - 7, triggers 8 / 9
    EXPECT_EQ(sizeof(Stadia_Data_t), 10u);
    EXPECT_EQ(offsetof(Stadia_Data_t, leftStickX), 4u);
    EXPECT_EQ(offsetof(Stadia_Data_t, leftTrigger), 8u);
    EXPECT_BIT_AT(Stadia_Data_t, capture, 2, 0x01);
    EXPECT_BIT_AT(Stadia_Data_t, assistant, 2, 0x02);
    EXPECT_BIT_AT(Stadia_Data_t, guide, 2, 0x10);
    EXPECT_BIT_AT(Stadia_Data_t, start, 2, 0x20);
    EXPECT_BIT_AT(Stadia_Data_t, back, 2, 0x40);
    EXPECT_BIT_AT(Stadia_Data_t, rightThumbClick, 2, 0x80);
    EXPECT_BIT_AT(Stadia_Data_t, leftThumbClick, 3, 0x01);
    EXPECT_BIT_AT(Stadia_Data_t, rightShoulder, 3, 0x02);
    EXPECT_BIT_AT(Stadia_Data_t, leftShoulder, 3, 0x04);
    EXPECT_BIT_AT(Stadia_Data_t, y, 3, 0x08);
    EXPECT_BIT_AT(Stadia_Data_t, x, 3, 0x10);
    EXPECT_BIT_AT(Stadia_Data_t, b, 3, 0x20);
    EXPECT_BIT_AT(Stadia_Data_t, a, 3, 0x40);
}

TEST(PdLoaderLayout, SliderTouchesRoundTrip)
{
    PDLoaderInputReport report;
    report.buttons3_slider1 = 0x05;
    for (uint32_t touches : {0u, 1u, 0x80000000u, 0xFFFFFFFFu, 0x12345678u, 0xAAAAAAAAu})
    {
        report.set_slider_touches(touches);
        EXPECT_EQ(report.slider_touches(), touches) << std::hex << touches;
        // the buttons sharing the first slider byte are kept
        EXPECT_EQ(report.buttons3_slider1 & 0x0F, 0x05);
    }
}

TEST(PdLoaderLayout, ReverseTouchBitsIsABitReversal)
{
    EXPECT_EQ(PDLoaderInputReport::reverse_touch_bits(0x00000001u), 0x80000000u);
    EXPECT_EQ(PDLoaderInputReport::reverse_touch_bits(0x80000000u), 0x00000001u);
    EXPECT_EQ(PDLoaderInputReport::reverse_touch_bits(0x0000000Fu), 0xF0000000u);
    EXPECT_EQ(PDLoaderInputReport::reverse_touch_bits(0x12345678u), 0x1E6A2C48u);
    for (uint32_t v : {0u, 0xFFFFFFFFu, 0xDEADBEEFu})
        EXPECT_EQ(PDLoaderInputReport::reverse_touch_bits(PDLoaderInputReport::reverse_touch_bits(v)), v);
}

TEST(Spice2xLayout, InputReport)
{
    // id, then little endian controller and panel fields
    EXPECT_EQ(offsetof(Spice2xInputReport, controller), 1u);
    EXPECT_EQ(offsetof(Spice2xInputReport, pad), 3u);
}
