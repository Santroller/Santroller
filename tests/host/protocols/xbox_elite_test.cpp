#include <gtest/gtest.h>
#include <vector>
#include "protocols/xbox_hid.hpp"
#include "protocols/proto_output.hpp"

// Xbox Elite Series 2 paddles over Bluetooth, from SDL's HIDAPI_DriverXboxOneBluetooth_HandleStatePacket.
// The GIP formats are covered end to end in gip/gip_input_test.cpp.

namespace
{
using Bytes = std::vector<uint8_t>;

bool pressed(const XboxHidState &s, GamepadButtonType b)
{
    proto_Output o = gamepad_button(b);
    return xbox_hid_tick_digital(s, o);
}

XboxHidState parse(const Bytes &r, uint16_t pid)
{
    XboxHidDescriptor desc;
    desc.init(nullptr, pid);
    XboxHidState s{};
    xbox_hid_parse_report(r.data(), r.size(), desc, s);
    return s;
}

Bytes report(size_t len, size_t paddle_at, uint8_t paddles, size_t profile_at, uint8_t profile)
{
    Bytes r(len, 0);
    r[0] = XBOX_HID_INPUT_REPORT_ID;
    r[13] = 0; // hat centred
    r[paddle_at] = paddles;
    r[profile_at] = profile;
    return r;
}

const GamepadButtonType PADDLES[] = {Gamepad_RightPaddle1, Gamepad_RightPaddle2, Gamepad_LeftPaddle1,
                                     Gamepad_LeftPaddle2};
} // namespace

TEST(XboxElite, BluetoothReportLayouts)
{
    struct
    {
        size_t len, paddle_at, profile_at;
    } layouts[] = {{55, 33, 35}, {39, 17, 19}, {20, 19, 17}};
    for (auto l : layouts)
    {
        for (int p = 0; p < 4; p++)
        {
            XboxHidState s = parse(report(l.len, l.paddle_at, 1 << p, l.profile_at, 0), XBOX_ELITE_2_BT_PID);
            for (int q = 0; q < 4; q++)
                EXPECT_EQ(pressed(s, PADDLES[q]), p == q) << "len " << l.len << " P" << p + 1 << " vs P" << q + 1;
            EXPECT_FALSE(pressed(s, Gamepad_A));
        }
        // a profile that maps the paddles presses other buttons instead
        XboxHidState s = parse(report(l.len, l.paddle_at, 0x0F, l.profile_at, 1), XBOX_ELITE_2_BLE_PID);
        for (auto b : PADDLES)
            EXPECT_FALSE(pressed(s, b)) << "len " << l.len;
    }
}

TEST(XboxElite, NoPaddlesOnOtherControllers)
{
    XboxHidState s = parse(report(20, 19, 0x0F, 17, 0), XBOX_SERIES_BT_PID);
    for (auto b : PADDLES)
        EXPECT_FALSE(pressed(s, b));
}

TEST(XboxElite, OnlyElitePidsHavePaddles)
{
    EXPECT_TRUE(xbox_elite_has_paddles(XBOX_VID, XBOX_ELITE_PID));
    EXPECT_TRUE(xbox_elite_has_paddles(XBOX_VID, XBOX_ELITE_2_PID));
    EXPECT_TRUE(xbox_elite_has_paddles(XBOX_VID, XBOX_ELITE_2_BT_PID));
    EXPECT_TRUE(xbox_elite_has_paddles(XBOX_VID, XBOX_ELITE_2_BLE_PID));
    EXPECT_FALSE(xbox_elite_has_paddles(XBOX_VID, XBOX_ONE_CONTROLLER_PID));
    EXPECT_FALSE(xbox_elite_has_paddles(0x0E6F, XBOX_ELITE_2_PID));
}
