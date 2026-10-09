#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "protocols/steam_controller.hpp"
#include "protocols/proto_output.hpp"

// Valve Steam Controller. Wired / dongle reports follow SDL's ValveInReport_t and Linux hid-steam,
// the BLE report follows SDL's steam controller BLE parser.

namespace
{
using Bytes = std::vector<uint8_t>;

bool pressed(const SteamControllerState &s, GamepadButtonType b)
{
    proto_Output o = gamepad_button(b);
    return steam_tick_digital(s, o);
}

uint16_t axis(const SteamControllerState &s, GamepadAxisType a)
{
    proto_Output o = gamepad_axis(a);
    return steam_tick_analog(s, o);
}

void put16(Bytes &r, size_t at, int16_t v)
{
    r[at] = (uint16_t)v & 0xFF;
    r[at + 1] = (uint16_t)v >> 8;
}

// 64 byte wired / dongle state report: version 0x0001, type 1 (controller state), length, packet number,
// 3 button bytes, left / right trigger, then left pad (or stick) and right pad
Bytes usb_report(uint32_t buttons, uint8_t lt, uint8_t rt, int16_t lx, int16_t ly, int16_t rx, int16_t ry)
{
    Bytes r(64, 0);
    r[0] = 0x01;
    r[1] = 0x00;
    r[2] = 0x01;
    r[3] = 0x3C;
    r[8] = buttons & 0xFF;
    r[9] = (buttons >> 8) & 0xFF;
    r[10] = (buttons >> 16) & 0xFF;
    r[11] = lt;
    r[12] = rt;
    put16(r, 16, lx);
    put16(r, 18, ly);
    put16(r, 20, rx);
    put16(r, 22, ry);
    return r;
}

// BLE state report: report id 3, 0xC0, then the chunk mask (low nibble of the first byte is the
// report type, 4 = state) followed by each chunk that is present, in mask order
Bytes ble_header(uint16_t chunks)
{
    return {0x03, 0xC0, (uint8_t)((chunks & 0xF0) | 0x04), (uint8_t)(chunks >> 8)};
}

constexpr uint16_t kBleButtons = 0x0010;
constexpr uint16_t kBleTriggers = 0x0020;
constexpr uint16_t kBleButtons3 = 0x0040;
constexpr uint16_t kBleStick = 0x0080;
constexpr uint16_t kBleLeftPad = 0x0100;
constexpr uint16_t kBleRightPad = 0x0200;
} // namespace

TEST(SteamController, DefaultStateIsNeutral)
{
    SteamControllerState s;
    EXPECT_EQ(axis(s, Gamepad_LeftStickX), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_LeftStickY), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_RightStickX), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_RightStickY), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_LeftTrigger), 0);
    EXPECT_EQ(axis(s, Gamepad_RightTrigger), 0);
    for (int b = Gamepad_A; b <= Gamepad_DpadRight; b++)
        EXPECT_FALSE(pressed(s, (GamepadButtonType)b)) << b;
}

TEST(SteamController, ButtonBitsMatchHidSteam)
{
    // bit positions from the table in Linux hid-steam (byte 8 bit 0 = bit 0 here)
    struct
    {
        uint32_t bit;
        GamepadButtonType button;
    } cases[] = {
        {1u << 2, Gamepad_RightShoulder}, {1u << 3, Gamepad_LeftShoulder}, {1u << 4, Gamepad_Y},
        {1u << 5, Gamepad_B},             {1u << 6, Gamepad_X},            {1u << 7, Gamepad_A},
        {1u << 8, Gamepad_DpadUp},        {1u << 9, Gamepad_DpadRight},    {1u << 10, Gamepad_DpadLeft},
        {1u << 11, Gamepad_DpadDown},     {1u << 12, Gamepad_Back},        {1u << 13, Gamepad_Guide},
        {1u << 14, Gamepad_Start},        {1u << 22, Gamepad_LeftThumbClick},
    };
    for (auto c : cases)
    {
        SteamControllerState s;
        s.buttons = c.bit;
        for (int b = Gamepad_A; b <= Gamepad_DpadRight; b++)
            EXPECT_EQ(pressed(s, (GamepadButtonType)b), b == c.button) << "bit " << std::hex << c.bit << " button " << std::dec << b;
    }
}

TEST(SteamController, GripsActAsShoulders)
{
    SteamControllerState s;
    s.buttons = STEAM_BTN_GRIP_L; // byte 9 bit 7, left back lever
    EXPECT_TRUE(pressed(s, Gamepad_LeftShoulder));
    EXPECT_FALSE(pressed(s, Gamepad_RightShoulder));
    s.buttons = STEAM_BTN_GRIP_R; // byte 10 bit 0, right back lever
    EXPECT_TRUE(pressed(s, Gamepad_RightShoulder));
    EXPECT_FALSE(pressed(s, Gamepad_LeftShoulder));
}

TEST(SteamController, NoCaptureButton)
{
    SteamControllerState s;
    s.buttons = 0xFFFFFF;
    EXPECT_FALSE(pressed(s, Gamepad_Capture));
}

// hid-steam: byte 10 bit 2 (bit 18 here) is "right-pad clicked" (BTN_THUMBR) and bit 7 (bit 23) is
// lpad_and_joy, a flag saying the left pad and stick are both in use. SDL agrees
// (STEAM_BUTTON_RIGHTPAD_CLICKED_MASK 0x40000, STEAM_LEFTPAD_AND_JOYSTICK_MASK 0x800000).
TEST(SteamController, RightPadClickIsRightThumbClick)
{
    SteamControllerState s;
    s.buttons = 1u << 18;
    EXPECT_TRUE(pressed(s, Gamepad_RightThumbClick));
    s.buttons = 1u << 23;
    EXPECT_FALSE(pressed(s, Gamepad_RightThumbClick));
}

TEST(SteamController, SticksAreSignedAroundTheCentre)
{
    SteamControllerState s;
    s.stick_x = -32768;
    s.stick_y = 32767;
    s.pad_x = 32767;
    s.pad_y = -32768;
    EXPECT_EQ(axis(s, Gamepad_LeftStickX), 0);
    EXPECT_EQ(axis(s, Gamepad_LeftStickY), 0xFFFF); // up is positive, like the firmware's axes
    EXPECT_EQ(axis(s, Gamepad_RightStickX), 0xFFFF);
    EXPECT_EQ(axis(s, Gamepad_RightStickY), 0);
    s.stick_x = s.stick_y = s.pad_x = s.pad_y = -1;
    EXPECT_EQ(axis(s, Gamepad_LeftStickX), 0x7FFF);
    EXPECT_EQ(axis(s, Gamepad_LeftStickY), 0x7FFF);
    EXPECT_EQ(axis(s, Gamepad_RightStickX), 0x7FFF);
    EXPECT_EQ(axis(s, Gamepad_RightStickY), 0x7FFF);
}

TEST(SteamController, TriggersScaleToFullRange)
{
    SteamControllerState s;
    s.trigger_l = 0xFF;
    s.trigger_r = 0x80;
    EXPECT_EQ(axis(s, Gamepad_LeftTrigger), 0xFFFF);
    EXPECT_EQ(axis(s, Gamepad_RightTrigger), 0x8080);
}

TEST(SteamController, TickHelpersIgnoreOtherMappings)
{
    SteamControllerState s;
    s.buttons = 0xFFFFFF;
    s.trigger_l = 0xFF;
    proto_Output b = non_gamepad_mapping(Gamepad_A);
    proto_Output a = non_gamepad_mapping(Gamepad_LeftTrigger);
    EXPECT_FALSE(steam_tick_digital(s, b));
    EXPECT_EQ(steam_tick_analog(s, a), 0);
    EXPECT_EQ(axis(s, Gamepad_AccelX), 0);
}

TEST(SteamController, ParsesWiredReport)
{
    SteamControllerState s;
    auto r = usb_report(STEAM_BTN_A | STEAM_BTN_GRIP_R | STEAM_BTN_STICK_CLICK, 0x12, 0xFE, -1000, 2000, 3000, -4000);
    ASSERT_TRUE(steam_parse_usb_report(r.data(), r.size(), s));
    EXPECT_EQ(s.buttons, STEAM_BTN_A | STEAM_BTN_GRIP_R | STEAM_BTN_STICK_CLICK);
    EXPECT_EQ(s.trigger_l, 0x12);
    EXPECT_EQ(s.trigger_r, 0xFE);
    EXPECT_EQ(s.stick_x, -1000);
    EXPECT_EQ(s.stick_y, 2000);
    EXPECT_EQ(s.pad_x, 3000);
    EXPECT_EQ(s.pad_y, -4000);
}

TEST(SteamController, WiredReportIgnoresByte11Upwards)
{
    // only three button bytes, byte 11 is the left trigger
    SteamControllerState s;
    auto r = usb_report(0, 0xFF, 0, 0, 0, 0, 0);
    ASSERT_TRUE(steam_parse_usb_report(r.data(), r.size(), s));
    EXPECT_EQ(s.buttons, 0u);
}

TEST(SteamController, RejectsOtherUsbReports)
{
    SteamControllerState s;
    s.buttons = STEAM_BTN_B;
    auto r = usb_report(STEAM_BTN_A, 0, 0, 0, 0, 0, 0);
    r[2] = 0x04; // battery status / other report types
    EXPECT_FALSE(steam_parse_usb_report(r.data(), r.size(), s));
    r[2] = 0x01;
    r[0] = 0x02;
    EXPECT_FALSE(steam_parse_usb_report(r.data(), r.size(), s));
    EXPECT_EQ(s.buttons, STEAM_BTN_B);
    EXPECT_FALSE(steam_parse_usb_report(r.data(), 13, s));
}

// a wired report is read up to byte 23, so anything shorter must be rejected without reading past the end
TEST(SteamController, ShortWiredReportIsRejectedWithoutOverreading)
{
    SteamControllerState s;
    auto full = usb_report(STEAM_BTN_A, 0, 0, 0, 0, 0, 0);
    for (size_t len = 14; len < 24; len++)
    {
        // exactly len bytes on the heap so ASan catches any read past the end
        std::unique_ptr<uint8_t[]> buf(new uint8_t[len]);
        memcpy(buf.get(), full.data(), len);
        EXPECT_FALSE(steam_parse_usb_report(buf.get(), len, s)) << len;
    }
}

TEST(SteamController, UsbParserForwardsBleEncapsulation)
{
    SteamControllerState s;
    Bytes r = ble_header(kBleButtons);
    r.insert(r.end(), {0x80, 0x00, 0x00}); // A
    r.resize(20, 0);
    ASSERT_TRUE(steam_parse_usb_report(r.data(), r.size(), s));
    EXPECT_TRUE(pressed(s, Gamepad_A));
}

TEST(SteamController, ParsesBleChunksInOrder)
{
    SteamControllerState s;
    Bytes r = ble_header(kBleButtons | kBleTriggers | kBleStick | kBleRightPad);
    r.insert(r.end(), {0x10, 0x20, 0x40});       // Y, Guide, stick click
    r.insert(r.end(), {0x33, 0x44});             // triggers
    r.insert(r.end(), {0x18, 0xFC, 0xD0, 0x07}); // stick -1000, 2000
    r.insert(r.end(), {0xB8, 0x0B, 0x60, 0xF0}); // right pad 3000, -4000
    ASSERT_TRUE(steam_parse_ble_report(r.data(), r.size(), s));
    EXPECT_EQ(s.buttons, 0x402010u);
    EXPECT_EQ(s.trigger_l, 0x33);
    EXPECT_EQ(s.trigger_r, 0x44);
    EXPECT_EQ(s.stick_x, -1000);
    EXPECT_EQ(s.stick_y, 2000);
    EXPECT_EQ(s.pad_x, 3000);
    EXPECT_EQ(s.pad_y, -4000);
}

TEST(SteamController, BleSkipsTheLeftPadChunk)
{
    SteamControllerState s;
    Bytes r = ble_header(kBleLeftPad | kBleRightPad);
    r.insert(r.end(), {0x11, 0x11, 0x22, 0x22}); // left pad, unused
    r.insert(r.end(), {0x01, 0x00, 0xFF, 0xFF}); // right pad 1, -1
    ASSERT_TRUE(steam_parse_ble_report(r.data(), r.size(), s));
    EXPECT_EQ(s.pad_x, 1);
    EXPECT_EQ(s.pad_y, -1);
    EXPECT_EQ(s.stick_x, 0);
}

TEST(SteamController, BleOnlyUpdatesChunksThatArePresent)
{
    SteamControllerState s;
    s.buttons = STEAM_BTN_X;
    s.stick_x = 1234;
    Bytes r = ble_header(kBleTriggers);
    r.insert(r.end(), {0xFF, 0x00});
    r.resize(10, 0);
    ASSERT_TRUE(steam_parse_ble_report(r.data(), r.size(), s));
    EXPECT_EQ(s.trigger_l, 0xFF);
    EXPECT_EQ(s.buttons, STEAM_BTN_X);
    EXPECT_EQ(s.stick_x, 1234);
}

TEST(SteamController, BleRejectsOtherReports)
{
    SteamControllerState s;
    Bytes r = ble_header(kBleButtons);
    r.insert(r.end(), {0x80, 0x00, 0x00});
    r.resize(20, 0);
    Bytes bad = r;
    bad[0] = 0x01;
    EXPECT_FALSE(steam_parse_ble_report(bad.data(), bad.size(), s));
    bad = r;
    bad[1] = 0x00;
    EXPECT_FALSE(steam_parse_ble_report(bad.data(), bad.size(), s));
    bad = r;
    bad[2] = (bad[2] & 0xF0) | 0x05; // not a state report
    EXPECT_FALSE(steam_parse_ble_report(bad.data(), bad.size(), s));
    EXPECT_FALSE(steam_parse_ble_report(r.data(), 9, s));
    EXPECT_EQ(s.buttons, 0u);
}

TEST(SteamController, BleTruncatedChunksAreNotRead)
{
    // every chunk claimed, but the report stops after the buttons: nothing past the end may be read
    Bytes r = ble_header(kBleButtons | kBleTriggers | kBleStick | kBleLeftPad | kBleRightPad);
    r.insert(r.end(), {0x80, 0x00, 0x00, 0x7F, 0x7F, 0x7F});
    std::unique_ptr<uint8_t[]> buf(new uint8_t[r.size()]);
    memcpy(buf.get(), r.data(), r.size());
    SteamControllerState s;
    ASSERT_TRUE(steam_parse_ble_report(buf.get(), r.size(), s));
    EXPECT_EQ(s.buttons, STEAM_BTN_A);
    EXPECT_EQ(s.trigger_l, 0x7F);
    EXPECT_EQ(s.trigger_r, 0x7F);
    EXPECT_EQ(s.stick_x, 0);
    EXPECT_EQ(s.pad_x, 0);
}

// SDL's BLE parser has a third button chunk (0x40, k_EBLEButtonChunk3: 3 more button bytes) between
// the triggers and the left stick
TEST(SteamController, BleSkipsTheThirdButtonChunk)
{
    SteamControllerState s;
    Bytes r = ble_header(kBleButtons | kBleTriggers | kBleButtons3 | kBleStick);
    r.insert(r.end(), {0x80, 0x00, 0x00});       // A
    r.insert(r.end(), {0x00, 0x00});             // triggers
    r.insert(r.end(), {0xAA, 0xBB, 0xCC});       // more buttons
    r.insert(r.end(), {0x18, 0xFC, 0xD0, 0x07}); // stick -1000, 2000
    ASSERT_TRUE(steam_parse_ble_report(r.data(), r.size(), s));
    EXPECT_EQ(s.stick_x, -1000);
    EXPECT_EQ(s.stick_y, 2000);
}

TEST(SteamController, CommandBuffers)
{
    // 0xC0, command, payload length, payload
    EXPECT_EQ(sizeof(STEAM_CMD_CLEAR_MAPPINGS_BUF), 3u);
    EXPECT_EQ(STEAM_CMD_CLEAR_MAPPINGS_BUF[1], 0x81);
    EXPECT_EQ(STEAM_CMD_DISABLE_LIZARD_BUF[1], 0x87);
    // length byte covers the register / value triplets that follow
    EXPECT_EQ(STEAM_CMD_DISABLE_LIZARD_BUF[2], sizeof(STEAM_CMD_DISABLE_LIZARD_BUF) - 3);
    EXPECT_EQ(STEAM_CMD_DISABLE_LIZARD_BUF[2] % 3, 0);
}

TEST(SteamController, BleUuids)
{
    // 100F6C32-1735-4313-B402-38567131E5F3 / 100F6C34-...
    const uint8_t service[16] = {0x10, 0x0F, 0x6C, 0x32, 0x17, 0x35, 0x43, 0x13,
                                 0xB4, 0x02, 0x38, 0x56, 0x71, 0x31, 0xE5, 0xF3};
    EXPECT_EQ(0, memcmp(STEAM_BLE_SERVICE_UUID, service, 16));
    uint8_t report[16];
    memcpy(report, service, 16);
    report[3] = 0x34;
    EXPECT_EQ(0, memcmp(STEAM_BLE_CHAR_REPORT_UUID, report, 16));
}
