#include <gtest/gtest.h>
#include <vector>
#include "protocols/steam_triton.hpp"
#include "protocols/proto_output.hpp"

// 2026 Steam Controller (Triton). Report layouts follow SDL's TritonMTUNoQuat_t / TritonMTUNoQuat32TS_t
// and the button table in SDL_hidapi_steam_triton.c.

namespace
{
using Bytes = std::vector<uint8_t>;

bool pressed(const SteamTritonState &s, GamepadButtonType b)
{
    proto_Output o = gamepad_button(b);
    return steam_triton_tick_digital(s, o);
}

uint16_t axis(const SteamTritonState &s, GamepadAxisType a)
{
    proto_Output o = gamepad_axis(a);
    return steam_triton_tick_analog(s, o);
}

void put16(Bytes &r, size_t at, int16_t v)
{
    r[at] = (uint16_t)v & 0xFF;
    r[at + 1] = (uint16_t)v >> 8;
}

// report id, seq, u32 buttons, triggers, sticks, then trackpads / IMU (left zeroed)
Bytes state_report(uint8_t id, uint32_t buttons, int16_t lt, int16_t rt, int16_t lx, int16_t ly, int16_t rx, int16_t ry)
{
    Bytes r(46, 0);
    r[0] = id;
    r[1] = 0x55;
    r[2] = buttons & 0xFF;
    r[3] = (buttons >> 8) & 0xFF;
    r[4] = (buttons >> 16) & 0xFF;
    r[5] = (buttons >> 24) & 0xFF;
    put16(r, 6, lt);
    put16(r, 8, rt);
    put16(r, 10, lx);
    put16(r, 12, ly);
    put16(r, 14, rx);
    put16(r, 16, ry);
    return r;
}
} // namespace

TEST(SteamTriton, DefaultStateIsNeutral)
{
    SteamTritonState s;
    EXPECT_EQ(axis(s, Gamepad_LeftStickX), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_LeftStickY), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_RightStickX), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_RightStickY), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_LeftTrigger), 0);
    EXPECT_EQ(axis(s, Gamepad_RightTrigger), 0);
    for (int b = Gamepad_A; b <= Gamepad_DpadRight; b++)
        EXPECT_FALSE(pressed(s, (GamepadButtonType)b)) << b;
}

TEST(SteamTriton, ButtonBitsMatchSdl)
{
    struct
    {
        uint32_t bit;
        GamepadButtonType button;
    } cases[] = {
        {0x00000001, Gamepad_A},          {0x00000002, Gamepad_B},
        {0x00000004, Gamepad_X},          {0x00000008, Gamepad_Y},
        {0x00000010, Gamepad_Capture},    {0x00000020, Gamepad_RightThumbClick},
        {0x00000040, Gamepad_Start},      {0x00000200, Gamepad_RightShoulder},
        {0x00000400, Gamepad_DpadDown},   {0x00000800, Gamepad_DpadRight},
        {0x00001000, Gamepad_DpadLeft},   {0x00002000, Gamepad_DpadUp},
        {0x00004000, Gamepad_Back},       {0x00008000, Gamepad_LeftThumbClick},
        {0x00010000, Gamepad_Guide},      {0x00080000, Gamepad_LeftShoulder},
    };
    for (auto c : cases)
    {
        SteamTritonState s;
        s.buttons = c.bit;
        for (int b = Gamepad_A; b <= Gamepad_DpadRight; b++)
            EXPECT_EQ(pressed(s, (GamepadButtonType)b), b == c.button) << "bit " << std::hex << c.bit << " button " << std::dec << b;
    }
}

TEST(SteamTriton, TouchAndPaddleBitsAreIgnored)
{
    // back paddles, capacitive touch and pad / trigger clicks
    SteamTritonState s;
    s.buttons = 0x00000080 | 0x00000100 | 0x00020000 | 0x00040000 | 0x3FF00000;
    for (int b = Gamepad_A; b <= Gamepad_DpadRight; b++)
        EXPECT_FALSE(pressed(s, (GamepadButtonType)b)) << b;
}

TEST(SteamTriton, ParsesAllStateReportIds)
{
    for (int id : {TRITON_REPORT_STATE, TRITON_REPORT_STATE_BLE, TRITON_REPORT_STATE_TIMESTAMP})
    {
        SteamTritonState s;
        Bytes r = state_report(id, TRITON_BTN_A | TRITON_BTN_STEAM, 32767, 16384, -32768, 32767, 1000, -1000);
        ASSERT_TRUE(steam_triton_parse_report(r.data(), r.size(), s)) << int(id);
        EXPECT_TRUE(pressed(s, Gamepad_A));
        EXPECT_TRUE(pressed(s, Gamepad_Guide));
        EXPECT_EQ(axis(s, Gamepad_LeftTrigger), 0xFFFF);
        EXPECT_EQ(axis(s, Gamepad_RightTrigger), 32769);
        EXPECT_EQ(axis(s, Gamepad_LeftStickX), 0x0000);
        EXPECT_EQ(axis(s, Gamepad_LeftStickY), 0xFFFF); // up is positive
        EXPECT_EQ(axis(s, Gamepad_RightStickX), 0x8000 + 1000);
        EXPECT_EQ(axis(s, Gamepad_RightStickY), 0x8000 - 1000);
    }
}

TEST(SteamTriton, UpperButtonByteIsParsed)
{
    SteamTritonState s;
    Bytes r = state_report(TRITON_REPORT_STATE, 0x08000000, 0, 0, 0, 0, 0, 0);
    ASSERT_TRUE(steam_triton_parse_report(r.data(), r.size(), s));
    EXPECT_EQ(s.buttons, 0x08000000u);
}

TEST(SteamTriton, RejectsOtherReports)
{
    SteamTritonState s;
    s.buttons = TRITON_BTN_A;
    for (int id : {TRITON_REPORT_BATTERY, TRITON_REPORT_WIRELESS, TRITON_REPORT_WIRELESS_X, 0x01})
    {
        Bytes r = state_report(id, TRITON_BTN_B, 0, 0, 0, 0, 0, 0);
        EXPECT_FALSE(steam_triton_parse_report(r.data(), r.size(), s)) << int(id);
    }
    Bytes shortReport = state_report(TRITON_REPORT_STATE, TRITON_BTN_B, 0, 0, 0, 0, 0, 0);
    EXPECT_FALSE(steam_triton_parse_report(shortReport.data(), 17, s));
    EXPECT_EQ(s.buttons, TRITON_BTN_A);
}

TEST(SteamTriton, NegativeTriggerClampsToZero)
{
    SteamTritonState s;
    s.trigger_l = -5;
    EXPECT_EQ(axis(s, Gamepad_LeftTrigger), 0);
}

TEST(SteamTriton, BlePayloadHasNoReportId)
{
    // BLE notifications on the Valve service carry the 45 byte state without its report ID
    for (int id : {TRITON_REPORT_STATE_BLE, TRITON_REPORT_STATE_TIMESTAMP})
    {
        Bytes r = state_report(id, TRITON_BTN_B | TRITON_BTN_DPAD_UP, 0, 32767, 100, -100, 0, 0);
        Bytes payload(r.begin() + 1, r.end());
        ASSERT_EQ(payload.size(), (size_t)TRITON_STATE_PAYLOAD_LEN);
        SteamTritonState s;
        ASSERT_TRUE(steam_triton_parse_state(payload.data(), payload.size(), s));
        EXPECT_TRUE(pressed(s, Gamepad_B));
        EXPECT_TRUE(pressed(s, Gamepad_DpadUp));
        EXPECT_EQ(axis(s, Gamepad_RightTrigger), 0xFFFF);
        EXPECT_EQ(axis(s, Gamepad_LeftStickX), 0x8000 + 100);
        EXPECT_EQ(axis(s, Gamepad_LeftStickY), 0x8000 - 100);
    }
    SteamTritonState s;
    Bytes tooShort(16, 0);
    EXPECT_FALSE(steam_triton_parse_state(tooShort.data(), tooShort.size(), s));
}

TEST(SteamTriton, DisableLizardMessage)
{
    // ID_SET_SETTINGS_VALUES, one 3 byte setting: SETTING_LIZARD_MODE = LIZARD_MODE_OFF
    const uint8_t expected[] = {0x87, 0x03, 0x09, 0x00, 0x00};
    ASSERT_EQ(sizeof(TRITON_DISABLE_LIZARD_MSG), sizeof(expected));
    EXPECT_EQ(memcmp(TRITON_DISABLE_LIZARD_MSG, expected, sizeof(expected)), 0);
}
