#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
#include "protocols/switch2.hpp"
#include "protocols/proto_output.hpp"

namespace
{
using Bytes = std::vector<uint8_t>;

// The BLE layout (USB adds a report ID in front), as SDL_hidapi_switch2.c and
// github.com/trevlars/switch2-controllers-linux read it: buttons at 4..7, then from 10 the left and
// right sticks as Nintendo's packed 12 bit pairs: x = b0 | (b1 & 0x0F) << 8, y = (b1 >> 4) | b2 << 4
Bytes report(uint32_t buttons, uint16_t lx, uint16_t ly, uint16_t rx, uint16_t ry)
{
    Bytes r(16, 0);
    r[4] = buttons & 0xFF;
    r[5] = (buttons >> 8) & 0xFF;
    r[6] = (buttons >> 16) & 0xFF;
    r[7] = (buttons >> 24) & 0xFF;
    r[10] = lx & 0xFF;
    r[11] = ((lx >> 8) & 0x0F) | ((ly & 0x0F) << 4);
    r[12] = ly >> 4;
    r[13] = rx & 0xFF;
    r[14] = ((rx >> 8) & 0x0F) | ((ry & 0x0F) << 4);
    r[15] = ry >> 4;
    return r;
}

void put16(Bytes &r, size_t at, int16_t v)
{
    r[at] = (uint16_t)v & 0xFF;
    r[at + 1] = (uint16_t)v >> 8;
}

Switch2ControllerState parse(const Bytes &r)
{
    Switch2ControllerState s;
    EXPECT_TRUE(switch2_parse_report(r.data(), r.size(), s));
    return s;
}

bool pressed(const Switch2ControllerState &s, GamepadButtonType b)
{
    proto_Output o = gamepad_button(b);
    return switch2_tick_digital(s, o);
}

uint16_t axis(const Switch2ControllerState &s, GamepadAxisType a)
{
    proto_Output o = gamepad_axis(a);
    return switch2_tick_analog(s, o);
}

int count_pressed(const Switch2ControllerState &s)
{
    const bool all[] = {s.a, s.b, s.x, s.y, s.l, s.r, s.zl, s.zr, s.minus, s.plus,
                        s.l3, s.r3, s.home, s.capture, s.dpad_up, s.dpad_down, s.dpad_left, s.dpad_right, s.sl, s.sr};
    int n = 0;
    for (bool b : all)
        n += b;
    return n;
}
} // namespace

TEST(Switch2, DefaultStateIsCentred)
{
    Switch2ControllerState s;
    EXPECT_EQ(count_pressed(s), 0);
    // 2048 is the middle of the 12 bit range, which scales to the middle of the 16 bit range
    EXPECT_EQ(axis(s, Gamepad_LeftStickX), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_LeftStickY), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_RightStickX), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_RightStickY), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_LeftTrigger), 0);
}

TEST(Switch2, RejectsShortReports)
{
    Switch2ControllerState s;
    s.a = true;
    Bytes r = report(0xFFFFFFFF, 0, 0, 0, 0);
    EXPECT_FALSE(switch2_parse_report(r.data(), 15, s));
    EXPECT_FALSE(switch2_parse_report(r.data(), 0, s));
    EXPECT_TRUE(s.a);
    EXPECT_EQ(s.left_stick_x, 2048);
}

TEST(Switch2, UnpacksTwelveBitSticks)
{
    auto s = parse(report(0, 0x123, 0xABC, 0xFFF, 0x000));
    EXPECT_EQ(s.left_stick_x, 0x123);
    EXPECT_EQ(s.left_stick_y, 0xABC);
    EXPECT_EQ(s.right_stick_x, 0xFFF);
    EXPECT_EQ(s.right_stick_y, 0x000);

    s = parse(report(0, 0x000, 0xFFF, 0x800, 0x7FF));
    EXPECT_EQ(s.left_stick_x, 0x000);
    EXPECT_EQ(s.left_stick_y, 0xFFF);
    EXPECT_EQ(s.right_stick_x, 0x800);
    EXPECT_EQ(s.right_stick_y, 0x7FF);
}

TEST(Switch2, SticksScaleToSixteenBits)
{
    auto s = parse(report(0, 0, 0xFFF, 0x800, 0x001));
    EXPECT_EQ(axis(s, Gamepad_LeftStickX), 0);
    EXPECT_EQ(axis(s, Gamepad_LeftStickY), 0xFFF0);
    EXPECT_EQ(axis(s, Gamepad_RightStickX), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_RightStickY), 0x0010);
}

TEST(Switch2, ButtonsDontLeakIntoSticks)
{
    auto s = parse(report(0xFFFFFFFF, 0x800, 0x800, 0x800, 0x800));
    EXPECT_EQ(s.left_stick_x, 0x800);
    EXPECT_EQ(s.right_stick_y, 0x800);
}

TEST(Switch2, EachKnownButtonBitSetsOneButton)
{
    const int known[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, 22, 23};
    for (int bit = 0; bit < 32; bit++)
    {
        bool is_known = std::find(std::begin(known), std::end(known), bit) != std::end(known);
        auto s = parse(report(1u << bit, 0x800, 0x800, 0x800, 0x800));
        EXPECT_EQ(count_pressed(s), is_known ? 1 : 0) << "bit " << bit;
    }
    auto all = parse(report(0xFFFFFFFF, 0, 0, 0, 0));
    EXPECT_EQ(count_pressed(all), 20);
    // and a report with nothing held releases everything
    Switch2ControllerState s = all;
    Bytes none = report(0, 0, 0, 0, 0);
    ASSERT_TRUE(switch2_parse_report(none.data(), none.size(), s));
    EXPECT_EQ(count_pressed(s), 0);
}

TEST(Switch2, SideButtonsActAsShoulders)
{
    Switch2ControllerState s;
    s.sl = true;
    EXPECT_TRUE(pressed(s, Gamepad_LeftShoulder));
    EXPECT_FALSE(pressed(s, Gamepad_RightShoulder));
    s = {};
    s.sr = true;
    EXPECT_TRUE(pressed(s, Gamepad_RightShoulder));
    EXPECT_FALSE(pressed(s, Gamepad_LeftShoulder));
}

TEST(Switch2, DigitalTriggersAreFullRange)
{
    Switch2ControllerState s;
    s.zl = true;
    EXPECT_EQ(axis(s, Gamepad_LeftTrigger), 0xFFFF);
    EXPECT_EQ(axis(s, Gamepad_RightTrigger), 0);
    s.zl = false;
    s.zr = true;
    EXPECT_EQ(axis(s, Gamepad_LeftTrigger), 0);
    EXPECT_EQ(axis(s, Gamepad_RightTrigger), 0xFFFF);
    // ZL / ZR are triggers, not buttons
    for (int b = Gamepad_A; b <= Gamepad_DpadRight; b++)
        EXPECT_FALSE(pressed(s, (GamepadButtonType)b)) << b;
}

TEST(Switch2, TickHelpersIgnoreOtherMappings)
{
    Switch2ControllerState s;
    s.a = true;
    s.zl = true;
    proto_Output b = non_gamepad_mapping(Gamepad_A);
    proto_Output a = non_gamepad_mapping(Gamepad_LeftTrigger);
    EXPECT_FALSE(switch2_tick_digital(s, b));
    EXPECT_EQ(switch2_tick_analog(s, a), 0);
    // no motion yet, so it reads as none
    EXPECT_EQ(axis(s, Gamepad_GyroY), MOTION_ZERO);
}

TEST(Switch2, StateMapsToTheMatchingGamepadButton)
{
    struct
    {
        bool Switch2ControllerState::*field;
        GamepadButtonType button;
    } cases[] = {
        {&Switch2ControllerState::a, Gamepad_A},
        {&Switch2ControllerState::b, Gamepad_B},
        {&Switch2ControllerState::x, Gamepad_X},
        {&Switch2ControllerState::y, Gamepad_Y},
        {&Switch2ControllerState::l, Gamepad_LeftShoulder},
        {&Switch2ControllerState::r, Gamepad_RightShoulder},
        {&Switch2ControllerState::minus, Gamepad_Back},
        {&Switch2ControllerState::plus, Gamepad_Start},
        {&Switch2ControllerState::l3, Gamepad_LeftThumbClick},
        {&Switch2ControllerState::r3, Gamepad_RightThumbClick},
        {&Switch2ControllerState::home, Gamepad_Guide},
        {&Switch2ControllerState::capture, Gamepad_Capture},
        {&Switch2ControllerState::dpad_up, Gamepad_DpadUp},
        {&Switch2ControllerState::dpad_down, Gamepad_DpadDown},
        {&Switch2ControllerState::dpad_left, Gamepad_DpadLeft},
        {&Switch2ControllerState::dpad_right, Gamepad_DpadRight},
    };
    for (auto c : cases)
    {
        Switch2ControllerState s;
        s.*c.field = true;
        for (int b = Gamepad_A; b <= Gamepad_DpadRight; b++)
            EXPECT_EQ(pressed(s, (GamepadButtonType)b), b == c.button) << "button " << b;
    }
}

// Acceleration then gyro from 0x30, at 4096 per g and a 34.8 rad/s full scale, read like SDL as
// x = x, y = z, z = -y
TEST(Switch2, ParsesMotion)
{
    Bytes r = report(0, 2048, 2048, 2048, 2048);
    r.resize(0x3F, 0);
    put16(r, 0x34, 4096);
    put16(r, 0x3A, 1000);
    Switch2ControllerState s;
    ASSERT_TRUE(switch2_parse_report(r.data(), r.size(), s));
    EXPECT_TRUE(s.has_imu);
    EXPECT_EQ(axis(s, Gamepad_AccelY), motion_accel_axis(1.0f));
    EXPECT_EQ(axis(s, Gamepad_AccelZ), MOTION_ZERO);
    float dps = 1000 * 34.8f / 32767 * 180 / (float)M_PI;
    EXPECT_NEAR(axis(s, Gamepad_GyroY), motion_gyro_axis(dps), 1);

    // the bias read from flash is taken off
    s.gyro_bias[2] = 1000 * 34.8f / 32767;
    EXPECT_NEAR(axis(s, Gamepad_GyroY), MOTION_ZERO, 1);
}

TEST(Switch2, ShortReportsHaveNoMotion)
{
    Switch2ControllerState s;
    Bytes r = report(0, 2048, 2048, 2048, 2048);
    ASSERT_TRUE(switch2_parse_report(r.data(), r.size(), s));
    EXPECT_FALSE(s.has_imu);
    EXPECT_EQ(axis(s, Gamepad_AccelY), MOTION_ZERO);
}

TEST(Switch2, ReadsTheGyroBiasFromFlash)
{
    uint8_t reply[SWITCH2_FLASH_REPLY_LEN] = {};
    const float bias[3] = {0.01f, -0.02f, 0.03f};
    memcpy(reply + 0x10 + 4, bias, sizeof(bias));
    float out[3];
    ASSERT_TRUE(switch2_parse_gyro_calibration(reply, sizeof(reply), out));
    EXPECT_FLOAT_EQ(out[1], -0.02f);
    // erased flash
    memset(reply + 0x10, 0xFF, SWITCH2_FLASH_READ_LEN);
    EXPECT_FALSE(switch2_parse_gyro_calibration(reply, sizeof(reply), out));
}

TEST(Switch2, InitCommandsStateTheirLength)
{
    // byte 5 is the length after the 8 byte header, which SDL sends as the whole command
    for (const Switch2Command &command : SWITCH2_USB_INIT)
    {
        EXPECT_EQ(command.len, command.data[5] + 8);
        EXPECT_EQ(command.data[1], 0x91);
    }
    // the gyro calibration block, little endian
    EXPECT_EQ(SWITCH2_READ_GYRO_CALIBRATION[12] | (SWITCH2_READ_GYRO_CALIBRATION[13] << 8) | (SWITCH2_READ_GYRO_CALIBRATION[14] << 16),
              SWITCH2_FLASH_GYRO_CALIBRATION);
}
