#include <gtest/gtest.h>
#include <vector>
#include "protocols/gamecube_adapter.hpp"
#include "protocols/proto_output.hpp"

namespace
{
using Bytes = std::vector<uint8_t>;

// A Nintendo adapter report, with every port empty
Bytes nintendo_report()
{
    Bytes r(GAMECUBE_ADAPTER_INPUT_REPORT_LEN, 0);
    r[0] = GAMECUBE_ADAPTER_INPUT_REPORT_ID;
    return r;
}

void set_slot(Bytes &r, int port, uint8_t status, uint8_t b1, uint8_t b2, std::vector<uint8_t> axes)
{
    uint8_t *slot = r.data() + 1 + port * 9;
    slot[0] = status;
    slot[1] = b1;
    slot[2] = b2;
    for (size_t i = 0; i < axes.size(); i++)
    {
        slot[3 + i] = axes[i];
    }
}

bool pressed(const GameCubePortState &port, GamepadButtonType button)
{
    auto out = gamepad_button(button);
    return gamecube_tick_digital(port, out);
}

uint16_t axis(const GameCubePortState &port, const GameCubeAxisRange &range, GamepadAxisType a)
{
    auto out = gamepad_axis(a);
    return gamecube_tick_analog(port, range, out);
}

TEST(GameCubeAdapter, NintendoReportFillsEveryPort)
{
    auto r = nintendo_report();
    set_slot(r, 0, GAMECUBE_ADAPTER_STATUS_WIRED | GAMECUBE_ADAPTER_STATUS_RUMBLE_POWER, 0x01 | 0x80, 0x02,
             {10, 20, 30, 40, 50, 60});
    set_slot(r, 2, GAMECUBE_ADAPTER_STATUS_WIRELESS, 0x08, 0x01 | 0x08, {});
    GameCubePortState ports[GAMECUBE_ADAPTER_PORTS];
    ASSERT_TRUE(gamecube_adapter_parse(r.data(), r.size(), ports));

    EXPECT_TRUE(ports[0].connected);
    EXPECT_FALSE(ports[0].wireless);
    EXPECT_TRUE(ports[0].rumble_power);
    EXPECT_EQ(ports[0].buttons, GAMECUBE_BUTTON_A | GAMECUBE_BUTTON_DPAD_UP | GAMECUBE_BUTTON_Z);
    EXPECT_EQ(ports[0].axes[GAMECUBE_AXIS_STICK_X], 10);
    EXPECT_EQ(ports[0].axes[GAMECUBE_AXIS_STICK_Y], 20);
    EXPECT_EQ(ports[0].axes[GAMECUBE_AXIS_C_STICK_X], 30);
    EXPECT_EQ(ports[0].axes[GAMECUBE_AXIS_C_STICK_Y], 40);
    EXPECT_EQ(ports[0].axes[GAMECUBE_AXIS_L], 50);
    EXPECT_EQ(ports[0].axes[GAMECUBE_AXIS_R], 60);

    EXPECT_FALSE(ports[1].connected);
    EXPECT_TRUE(ports[2].connected);
    EXPECT_TRUE(ports[2].wireless);
    EXPECT_EQ(ports[2].buttons, GAMECUBE_BUTTON_Y | GAMECUBE_BUTTON_START | GAMECUBE_BUTTON_L);
    EXPECT_FALSE(ports[3].connected);
}

TEST(GameCubeAdapter, NintendoReportButtonBits)
{
    struct Case
    {
        uint8_t b1, b2;
        uint16_t button;
    } cases[] = {
        {0x01, 0, GAMECUBE_BUTTON_A},
        {0x02, 0, GAMECUBE_BUTTON_B},
        {0x04, 0, GAMECUBE_BUTTON_X},
        {0x08, 0, GAMECUBE_BUTTON_Y},
        {0x10, 0, GAMECUBE_BUTTON_DPAD_LEFT},
        {0x20, 0, GAMECUBE_BUTTON_DPAD_RIGHT},
        {0x40, 0, GAMECUBE_BUTTON_DPAD_DOWN},
        {0x80, 0, GAMECUBE_BUTTON_DPAD_UP},
        {0, 0x01, GAMECUBE_BUTTON_START},
        {0, 0x02, GAMECUBE_BUTTON_Z},
        {0, 0x04, GAMECUBE_BUTTON_R},
        {0, 0x08, GAMECUBE_BUTTON_L},
    };
    for (const auto &c : cases)
    {
        auto r = nintendo_report();
        set_slot(r, 1, GAMECUBE_ADAPTER_STATUS_WIRED, c.b1, c.b2, {});
        GameCubePortState ports[GAMECUBE_ADAPTER_PORTS];
        ASSERT_TRUE(gamecube_adapter_parse(r.data(), r.size(), ports));
        EXPECT_EQ(ports[1].buttons, c.button) << int(c.b1) << " " << int(c.b2);
    }
}

TEST(GameCubeAdapter, NintendoRejectsOtherReports)
{
    GameCubePortState ports[GAMECUBE_ADAPTER_PORTS];
    auto r = nintendo_report();
    r[0] = 0x22;
    EXPECT_FALSE(gamecube_adapter_parse(r.data(), r.size(), ports));
    r = nintendo_report();
    EXPECT_FALSE(gamecube_adapter_parse(r.data(), r.size() - 1, ports));
}

TEST(GameCubeAdapter, EvoretroOlderFirmwareNamesThePort)
{
    // port 3, X and Z, start and dpad left
    Bytes r = {3, 0x01 | 0x80, 0x02 | 0x80, 100, 200, 70, 80, 90, 110, 0};
    GameCubePortState ports[GAMECUBE_ADAPTER_PORTS];
    uint8_t port = 0xFF;
    ASSERT_TRUE(evoretro_parse(r.data(), r.size(), ports, port));
    EXPECT_EQ(port, 2);
    EXPECT_TRUE(ports[2].connected);
    EXPECT_FALSE(ports[0].connected);
    EXPECT_EQ(ports[2].buttons, GAMECUBE_BUTTON_X | GAMECUBE_BUTTON_Z | GAMECUBE_BUTTON_START | GAMECUBE_BUTTON_DPAD_LEFT);
    EXPECT_EQ(ports[2].axes[GAMECUBE_AXIS_STICK_X], 100);
    EXPECT_EQ(ports[2].axes[GAMECUBE_AXIS_STICK_Y], 0xFF - 200);
    // C stick Y first, and X backwards on this firmware
    EXPECT_EQ(ports[2].axes[GAMECUBE_AXIS_C_STICK_X], 0xFF - 80);
    EXPECT_EQ(ports[2].axes[GAMECUBE_AXIS_C_STICK_Y], 70);
    EXPECT_EQ(ports[2].axes[GAMECUBE_AXIS_L], 90);
    EXPECT_EQ(ports[2].axes[GAMECUBE_AXIS_R], 110);
}

TEST(GameCubeAdapter, EvoretroNewerFirmwareIsOnePort)
{
    // A, B, Y, L and R clicks, dpad up, right and down
    Bytes r = {0x02 | 0x04 | 0x08 | 0x10 | 0x20, 0x10 | 0x20 | 0x40, 100, 200, 70, 80, 90, 110};
    r.push_back(0);
    GameCubePortState ports[GAMECUBE_ADAPTER_PORTS];
    uint8_t port = 0xFF;
    ASSERT_TRUE(evoretro_parse(r.data(), r.size(), ports, port));
    EXPECT_EQ(port, 0);
    EXPECT_EQ(ports[0].buttons, GAMECUBE_BUTTON_A | GAMECUBE_BUTTON_B | GAMECUBE_BUTTON_Y | GAMECUBE_BUTTON_L |
                                    GAMECUBE_BUTTON_R | GAMECUBE_BUTTON_DPAD_UP | GAMECUBE_BUTTON_DPAD_RIGHT |
                                    GAMECUBE_BUTTON_DPAD_DOWN);
    // C stick Y backwards on this firmware instead
    EXPECT_EQ(ports[0].axes[GAMECUBE_AXIS_C_STICK_X], 80);
    EXPECT_EQ(ports[0].axes[GAMECUBE_AXIS_C_STICK_Y], 0xFF - 70);
}

TEST(GameCubeAdapter, EvoretroRejectsBadReports)
{
    GameCubePortState ports[GAMECUBE_ADAPTER_PORTS];
    uint8_t port;
    Bytes bad_port = {5, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_FALSE(evoretro_parse(bad_port.data(), bad_port.size(), ports, port));
    bad_port[0] = 0;
    EXPECT_FALSE(evoretro_parse(bad_port.data(), bad_port.size(), ports, port));
    Bytes short_report(8, 0);
    EXPECT_FALSE(evoretro_parse(short_report.data(), short_report.size(), ports, port));
    for (const auto &p : ports)
    {
        EXPECT_FALSE(p.connected);
    }
}

TEST(GameCubeAdapter, RangeStartsAtTheUsualTravelAndWidens)
{
    GameCubePortState port;
    GameCubeAxisRange range;
    port.axes[GAMECUBE_AXIS_STICK_X] = 128;
    EXPECT_NEAR(axis(port, range, Gamepad_LeftStickX), 32768, 400);
    port.axes[GAMECUBE_AXIS_STICK_X] = 128 + 88;
    EXPECT_EQ(axis(port, range, Gamepad_LeftStickX), UINT16_MAX);
    port.axes[GAMECUBE_AXIS_STICK_X] = 128 - 88;
    EXPECT_EQ(axis(port, range, Gamepad_LeftStickX), 0);

    // a stick that reaches further widens the range, so it only hits the end at its own limit
    port.axes[GAMECUBE_AXIS_STICK_X] = 240;
    range.update(port);
    EXPECT_EQ(axis(port, range, Gamepad_LeftStickX), UINT16_MAX);
    port.axes[GAMECUBE_AXIS_STICK_X] = 128 + 88;
    EXPECT_LT(axis(port, range, Gamepad_LeftStickX), UINT16_MAX);

    range.reset();
    EXPECT_EQ(axis(port, range, Gamepad_LeftStickX), UINT16_MAX);
}

TEST(GameCubeAdapter, TriggerClickIsAllTheWayIn)
{
    GameCubePortState port;
    GameCubeAxisRange range;
    port.axes[GAMECUBE_AXIS_L] = 150;
    port.axes[GAMECUBE_AXIS_R] = 20;
    range.update(port);
    EXPECT_LT(axis(port, range, Gamepad_LeftTrigger), UINT16_MAX);
    EXPECT_EQ(axis(port, range, Gamepad_RightTrigger), 0);
    port.buttons = GAMECUBE_BUTTON_L;
    EXPECT_EQ(axis(port, range, Gamepad_LeftTrigger), UINT16_MAX);
    EXPECT_EQ(axis(port, range, Gamepad_RightTrigger), 0);
}

TEST(GameCubeAdapter, ButtonsByLegend)
{
    GameCubePortState port;
    port.buttons = GAMECUBE_BUTTON_Z;
    EXPECT_TRUE(pressed(port, Gamepad_RightShoulder));
    EXPECT_FALSE(pressed(port, Gamepad_LeftShoulder));
    port.buttons = GAMECUBE_BUTTON_B;
    EXPECT_TRUE(pressed(port, Gamepad_B));
    EXPECT_FALSE(pressed(port, Gamepad_A));
    // the trigger clicks only show up on the trigger axes
    port.buttons = GAMECUBE_BUTTON_L | GAMECUBE_BUTTON_R;
    EXPECT_FALSE(pressed(port, Gamepad_LeftShoulder));
    EXPECT_FALSE(pressed(port, Gamepad_RightShoulder));

    port.buttons = 0xFFFF;
    auto other = non_gamepad_mapping(Gamepad_A);
    EXPECT_FALSE(gamecube_tick_digital(port, other));
    GameCubeAxisRange range;
    other = non_gamepad_mapping(Gamepad_LeftStickX);
    EXPECT_EQ(gamecube_tick_analog(port, range, other), 0);
}

TEST(GameCubeAdapter, NintendoRumbleNeedsPowerAndAWiredController)
{
    GameCubePortState ports[GAMECUBE_ADAPTER_PORTS];
    for (auto &p : ports)
    {
        p.connected = true;
        p.rumble_power = true;
    }
    ports[1].wireless = true;
    ports[2].rumble_power = false;
    ports[3].connected = false;
    bool rumble[GAMECUBE_ADAPTER_PORTS] = {true, true, true, true};
    uint8_t out[GAMECUBE_ADAPTER_RUMBLE_REPORT_LEN];
    gamecube_adapter_rumble_report(rumble, ports, out);
    EXPECT_EQ(out[0], GAMECUBE_ADAPTER_RUMBLE_REPORT_ID);
    EXPECT_EQ(out[1], 1);
    EXPECT_EQ(out[2], 0);
    EXPECT_EQ(out[3], 0);
    EXPECT_EQ(out[4], 0);

    rumble[0] = false;
    gamecube_adapter_rumble_report(rumble, ports, out);
    EXPECT_EQ(out[1], 0);
}

TEST(GameCubeAdapter, EvoretroRumbleIsOnOrOff)
{
    uint8_t out[EVORETRO_RUMBLE_REPORT_LEN];
    evoretro_rumble_report(true, out);
    EXPECT_EQ(out[0], 0xFF);
    EXPECT_EQ(out[1], 0xFF);
    evoretro_rumble_report(false, out);
    EXPECT_EQ(out[0], 0x00);
    EXPECT_EQ(out[1], 0x00);
}

// The emulated adapter's report has to read back as the controller it was built from
TEST(GameCubeAdapter, EmulatedReportParsesBack)
{
    GameCubeGamepad_Data_t in = {};
    in.a = in.y = in.start = in.z = in.l = in.dpadUp = in.dpadRight = 1;
    in.alwaysOne = 1;
    in.leftStickX = 1;
    in.leftStickY = 2;
    in.cStickX = 3;
    in.cStickY = 4;
    in.leftTrigger = 5;
    in.rightTrigger = 6;
    uint8_t report[GAMECUBE_ADAPTER_INPUT_REPORT_LEN];
    gamecube_adapter_build_report(&in, 1, report);

    GameCubePortState ports[GAMECUBE_ADAPTER_PORTS];
    ASSERT_TRUE(gamecube_adapter_parse(report, sizeof(report), ports));
    EXPECT_TRUE(ports[0].connected);
    EXPECT_FALSE(ports[0].wireless);
    EXPECT_TRUE(ports[0].rumble_power);
    EXPECT_EQ(ports[0].buttons, GAMECUBE_BUTTON_A | GAMECUBE_BUTTON_Y | GAMECUBE_BUTTON_START | GAMECUBE_BUTTON_Z |
                                    GAMECUBE_BUTTON_L | GAMECUBE_BUTTON_DPAD_UP | GAMECUBE_BUTTON_DPAD_RIGHT);
    for (uint8_t i = 0; i < GAMECUBE_AXIS_COUNT; i++)
    {
        EXPECT_EQ(ports[0].axes[i], i + 1);
    }
    for (int port = 1; port < GAMECUBE_ADAPTER_PORTS; port++)
    {
        EXPECT_FALSE(ports[port].connected) << port;
    }
}

TEST(GameCubeAdapter, EmulatedPortsFillInOrder)
{
    GameCubeGamepad_Data_t in[2] = {};
    in[0].a = 1;
    in[1].b = 1;
    in[1].leftStickX = 99;
    uint8_t report[GAMECUBE_ADAPTER_INPUT_REPORT_LEN];
    gamecube_adapter_build_report(in, 2, report);

    GameCubePortState ports[GAMECUBE_ADAPTER_PORTS];
    ASSERT_TRUE(gamecube_adapter_parse(report, sizeof(report), ports));
    EXPECT_TRUE(ports[0].connected);
    EXPECT_EQ(ports[0].buttons, GAMECUBE_BUTTON_A);
    EXPECT_TRUE(ports[1].connected);
    EXPECT_EQ(ports[1].buttons, GAMECUBE_BUTTON_B);
    EXPECT_EQ(ports[1].axes[GAMECUBE_AXIS_STICK_X], 99);
    EXPECT_FALSE(ports[2].connected);
    EXPECT_FALSE(ports[3].connected);
}

TEST(GameCubeAdapter, EmulatedButtonsLandOnTheirOwnBits)
{
    // Each button alone, so a swapped pair shows up
    struct Case
    {
        void (*set)(GameCubeGamepad_Data_t &);
        uint16_t button;
    };
    const Case cases[] = {
        {[](GameCubeGamepad_Data_t &r) { r.a = 1; }, GAMECUBE_BUTTON_A},
        {[](GameCubeGamepad_Data_t &r) { r.b = 1; }, GAMECUBE_BUTTON_B},
        {[](GameCubeGamepad_Data_t &r) { r.x = 1; }, GAMECUBE_BUTTON_X},
        {[](GameCubeGamepad_Data_t &r) { r.y = 1; }, GAMECUBE_BUTTON_Y},
        {[](GameCubeGamepad_Data_t &r) { r.start = 1; }, GAMECUBE_BUTTON_START},
        {[](GameCubeGamepad_Data_t &r) { r.z = 1; }, GAMECUBE_BUTTON_Z},
        {[](GameCubeGamepad_Data_t &r) { r.l = 1; }, GAMECUBE_BUTTON_L},
        {[](GameCubeGamepad_Data_t &r) { r.r = 1; }, GAMECUBE_BUTTON_R},
        {[](GameCubeGamepad_Data_t &r) { r.dpadUp = 1; }, GAMECUBE_BUTTON_DPAD_UP},
        {[](GameCubeGamepad_Data_t &r) { r.dpadDown = 1; }, GAMECUBE_BUTTON_DPAD_DOWN},
        {[](GameCubeGamepad_Data_t &r) { r.dpadLeft = 1; }, GAMECUBE_BUTTON_DPAD_LEFT},
        {[](GameCubeGamepad_Data_t &r) { r.dpadRight = 1; }, GAMECUBE_BUTTON_DPAD_RIGHT},
    };
    for (const auto &c : cases)
    {
        GameCubeGamepad_Data_t in = {};
        c.set(in);
        uint8_t report[GAMECUBE_ADAPTER_INPUT_REPORT_LEN];
        gamecube_adapter_build_report(&in, 1, report);
        GameCubePortState ports[GAMECUBE_ADAPTER_PORTS];
        ASSERT_TRUE(gamecube_adapter_parse(report, sizeof(report), ports));
        EXPECT_EQ(ports[0].buttons, c.button);
    }
}

TEST(GameCubeAdapter, EmulatedReportDescriptorMatchesTheAdapter)
{
    // Four bytes of usage, then ten reports of 21 bytes: the real adapter's 214
    EXPECT_EQ(sizeof(desc_hid_report_gamecube_adapter), 214u);
    // Report 0x21 is the second, an input
    const uint8_t *input = desc_hid_report_gamecube_adapter + 4 + 21;
    EXPECT_EQ(input[3], GAMECUBE_ADAPTER_INPUT_REPORT_ID);
    EXPECT_EQ(input[18], 0x81);
}
} // namespace
