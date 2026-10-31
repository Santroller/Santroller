#include <gtest/gtest.h>
#include "mappings/mapping_test_support.hpp"
#include "protocols/wheel.hpp"

// Wheels send a gamepad report: steering on the left stick, the accelerator and brake on the
// triggers (Microsoft's XInput wheel layout). PC HID gives the pedals 16 bit axes instead.
namespace
{
class WheelAxis : public MappingTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile(Wheel);

    // A full range axis, so the raw value is also the calibrated value. Everything but the
    // steering rests at its minimum.
    Driven<WheelAxisMapping> axis(WheelAxisType type, uint16_t value)
    {
        proto_Mapping config = type == Wheel_Steering ? stick_config() : trigger_config();
        config.mapping.which_mapping = proto_Output_wheelAxis_tag;
        config.mapping.mapping.wheelAxis = type;
        auto driven = drive<WheelAxisMapping>(config, profile);
        driven.set_analog(value);
        return driven;
    }
};
} // namespace

TEST_F(WheelAxis, XInputFollowsTheWheelLayout)
{
    Report<XInputGamepad_Data_t> report;
    axis(Wheel_Steering, 0)->update_xinput(report.buf());
    axis(Wheel_Accelerator, UINT16_MAX)->update_xinput(report.buf());
    axis(Wheel_Brake, 0x8000)->update_xinput(report.buf());
    EXPECT_EQ(report->leftStickX, INT16_MIN);
    EXPECT_EQ(report->rightTrigger, 0xFF);
    EXPECT_EQ(report->leftTrigger, 0x80);
}

TEST_F(WheelAxis, XInputClutchAndHandbrakeUseTheSpareStickAxes)
{
    Report<XInputGamepad_Data_t> report;
    axis(Wheel_Clutch, UINT16_MAX)->update_xinput(report.buf());
    axis(Wheel_Handbrake, UINT16_MAX)->update_xinput(report.buf());
    EXPECT_EQ(report->leftStickY, INT16_MAX);
    EXPECT_EQ(report->rightStickY, INT16_MAX);
}

TEST_F(WheelAxis, HidPedalsAreSixteenBit)
{
    Report<PCGamepadDpad_Data_t> report;
    axis(Wheel_Steering, UINT16_MAX)->update_hid(report.buf());
    axis(Wheel_Accelerator, UINT16_MAX)->update_hid(report.buf());
    axis(Wheel_Brake, 1)->update_hid(report.buf());
    axis(Wheel_Clutch, 0x8000)->update_hid(report.buf());
    axis(Wheel_Handbrake, UINT16_MAX)->update_hid(report.buf());
    EXPECT_EQ(report->leftStickX, INT16_MAX);
    EXPECT_EQ(report->leftStickY, INT16_MAX);
    EXPECT_EQ(report->rightStickX, INT16_MIN + 1);
    EXPECT_EQ(report->rightStickY, 0);
    EXPECT_EQ(report->leftTrigger, 0xFF);
    EXPECT_EQ(report->rightTrigger, 0);
}

// A pedal at rest leaves the report alone, which the HID and XInput devices start at the minimum
TEST_F(WheelAxis, ReleasedPedalsLeaveTheReportAlone)
{
    Report<PCGamepadDpad_Data_t> report;
    report->leftStickY = INT16_MIN;
    axis(Wheel_Accelerator, 0)->update_hid(report.buf());
    EXPECT_EQ(report->leftStickY, INT16_MIN);
}

TEST_F(WheelAxis, ConsolesGetSteeringAndPedalsOnly)
{
    Report<PS2Gamepad_Data_t> report;
    axis(Wheel_Steering, UINT16_MAX)->update_ps2(report.buf());
    axis(Wheel_Accelerator, UINT16_MAX)->update_ps2(report.buf());
    axis(Wheel_Clutch, UINT16_MAX)->update_ps2(report.buf());
    axis(Wheel_Handbrake, UINT16_MAX)->update_ps2(report.buf());
    EXPECT_EQ(report->leftStickX, 0xFF);
    EXPECT_EQ(report->rightTrigger, 0xFF);
    EXPECT_TRUE(report->r2);
    EXPECT_EQ(report->leftStickY, 0);
    EXPECT_EQ(report->rightStickY, 0);
}

// PS3 wheels are G27s, with 14 bit steering and 8 bit pedals that read 0xFF released
TEST_F(WheelAxis, PS3UsesTheG27Report)
{
    Report<PS3G27_Data_t> report;
    axis(Wheel_Steering, UINT16_MAX)->update_ps3(report.buf());
    axis(Wheel_Accelerator, UINT16_MAX)->update_ps3(report.buf());
    axis(Wheel_Brake, 0x8000)->update_ps3(report.buf());
    axis(Wheel_Clutch, 0x0100)->update_ps3(report.buf());
    axis(Wheel_Handbrake, UINT16_MAX)->update_ps3(report.buf());
    // steering is the top 14 bits of bytes 3 - 4 (GIMX's buttonsAndWheel)
    EXPECT_EQ(report.bytes()[3], 0xFC);
    EXPECT_EQ(report.bytes()[4], 0xFF);
    EXPECT_EQ(report->accelerator, 0);
    EXPECT_EQ(report->brake, 0x7F);
    EXPECT_EQ(report->clutch, 0xFE);
    EXPECT_EQ(report->shifterStatus, 0);
}

// PS4 wheels are G29s, with their own 16 bit steering and pedals that read 0xFFFF released
TEST_F(WheelAxis, PS4UsesTheG29Report)
{
    Report<PS4Wheel_Data_t> report;
    axis(Wheel_Steering, 0x1234)->update_ps4(report.buf());
    axis(Wheel_Accelerator, UINT16_MAX)->update_ps4(report.buf());
    axis(Wheel_Brake, 0x0100)->update_ps4(report.buf());
    axis(Wheel_Clutch, 0x8000)->update_ps4(report.buf());
    EXPECT_EQ(report->wheel.steering, 0x1234);
    EXPECT_EQ(report->wheel.accelerator, 0);
    EXPECT_EQ(report->wheel.brake, 0xFEFF);
    EXPECT_EQ(report->wheel.clutch, 0x7FFF);
    // nothing written over the sticks or triggers
    EXPECT_EQ(report->leftStickX, 0);
    EXPECT_EQ(report->rightTrigger, 0);
}

// No PS5 wheel to copy, so it is the PS4 wheel data where a PS5 guitar keeps its pickup
TEST_F(WheelAxis, PS5UsesTheG29DataToo)
{
    Report<PS5Wheel_Data_t> report;
    axis(Wheel_Steering, 0x1234)->update_ps5(report.buf());
    axis(Wheel_Brake, UINT16_MAX)->update_ps5(report.buf());
    EXPECT_EQ(report.bytes()[40], 0x34);
    EXPECT_EQ(report.bytes()[41], 0x12);
    EXPECT_EQ(report->wheel.brake, 0);
    EXPECT_EQ(report->leftStickX, 0);
}

TEST_F(WheelAxis, PS4HasNoHandbrake)
{
    Report<PS4Wheel_Data_t> report;
    axis(Wheel_Handbrake, UINT16_MAX)->update_ps4(report.buf());
    Report<PS4Wheel_Data_t> empty;
    EXPECT_EQ(memcmp(report.bytes(), empty.bytes(), sizeof(PS4Wheel_Data_t)), 0);
}

TEST_F(WheelAxis, PS4ReleasedPedalsLeaveTheReportAlone)
{
    Report<PS4Wheel_Data_t> report;
    report->wheel.accelerator = UINT16_MAX;
    axis(Wheel_Accelerator, 0)->update_ps4(report.buf());
    EXPECT_EQ(report->wheel.accelerator, UINT16_MAX);
}

namespace
{
class WheelButton : public MappingTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile(Wheel);

    Driven<WheelButtonMapping> pressed(WheelButtonType type, bool value = true)
    {
        proto_Mapping config = base_config();
        config.mapping.which_mapping = proto_Output_wheelButton_tag;
        config.mapping.mapping.wheelButton = type;
        auto driven = drive<WheelButtonMapping>(config, profile);
        driven.set_digital(value);
        return driven;
    }
};
} // namespace

TEST_F(WheelButton, PS4SetsTheG29Bits)
{
    Report<PS4Wheel_Data_t> report;
    pressed(Wheel_Gear1)->update_ps4(report.buf());
    pressed(Wheel_GearReverse)->update_ps4(report.buf());
    pressed(Wheel_Plus)->update_ps4(report.buf());
    pressed(Wheel_DialEnter)->update_ps4(report.buf());
    pressed(Wheel_Minus, false)->update_ps4(report.buf());
    // GIMX: gears 1-6 are bits 0-5 and reverse bit 7; enter 0x01, dial down 0x02, dial up 0x04,
    // minus 0x08, plus 0x10
    EXPECT_EQ(report.bytes()[51], 0x81);
    EXPECT_EQ(report.bytes()[54], 0x11);
}

TEST_F(WheelButton, PS5SetsTheSameBits)
{
    Report<PS5Wheel_Data_t> report;
    pressed(Wheel_Gear6)->update_ps5(report.buf());
    pressed(Wheel_DialUp)->update_ps5(report.buf());
    EXPECT_EQ(report.bytes()[48], 0x20);
    EXPECT_EQ(report.bytes()[51], 0x04);
}

// An Xbox One wheel reports which gear it is in, -1 for reverse
TEST_F(WheelButton, XboxOneReportsTheGear)
{
    Report<XboxOneWheel_Data_t> report;
    pressed(Wheel_Gear2, false)->update_xboxone(report.buf());
    EXPECT_EQ(report->gear, 0);
    pressed(Wheel_Gear3)->update_xboxone(report.buf());
    EXPECT_EQ(report->gear, 3);
    pressed(Wheel_GearReverse)->update_xboxone(report.buf());
    EXPECT_EQ(report->gear, -1);
    // the G29's own buttons have nowhere to go
    Report<XboxOneWheel_Data_t> other;
    pressed(Wheel_Plus)->update_xboxone(other.buf());
    EXPECT_EQ(other->gear, 0);
}

// A G27 sets its gear's button and moves its shifter's stick there (from GIMX's g27_ps3)
TEST_F(WheelButton, PS3MovesTheG27Shifter)
{
    Report<PS3G27_Data_t> report;
    pressed(Wheel_Gear5)->update_ps3(report.buf());
    EXPECT_EQ(report.bytes()[2], 0x10);
    EXPECT_EQ(report->shifterX, G27_SHIFTER_RIGHT);
    EXPECT_EQ(report->shifterY, G27_SHIFTER_UP);

    Report<PS3G27_Data_t> reverse;
    reverse->shifterStatus = G27_SHIFTER_DEFAULT_STATUS;
    pressed(Wheel_GearReverse)->update_ps3(reverse.buf());
    EXPECT_EQ(reverse.bytes()[2], 0);
    EXPECT_EQ(reverse->shifterY, G27_SHIFTER_DOWN);
    EXPECT_EQ(reverse->shifterStatus, G27_SHIFTER_DEFAULT_STATUS | G27_SHIFTER_REVERSE);
}

TEST_F(WheelButton, PS3GamepadButtonsUseTheG27Layout)
{
    auto press = [this](GamepadButtonType button, Report<PS3G27_Data_t> &report) {
        auto driven = drive<WheelGamepadButtonMapping>(gamepad_button(button), profile);
        driven.set_digital(true);
        driven->update_ps3(report.buf());
    };
    Report<PS3G27_Data_t> report;
    press(Gamepad_A, report);
    press(Gamepad_Y, report);
    press(Gamepad_DpadLeft, report);
    press(Gamepad_RightShoulder, report);
    press(Gamepad_Start, report);
    press(Gamepad_Guide, report);
    // GIMX: cross 0x10, triangle 0x80 over the hat; R1 0x01, start 0x20. No PS button
    EXPECT_EQ(report.bytes()[0], 0x90 | 0x04);
    EXPECT_EQ(report.bytes()[1], 0x21);
    EXPECT_EQ(report.bytes()[2], 0);
}

// A Speed Force's paddles are its analog triggers, the rest are the controller's buttons
TEST_F(WheelButton, GameCubePaddlesAreTheTriggers)
{
    auto press = [this](GamepadButtonType button, Report<GameCubeGamepad_Data_t> &report) {
        auto driven = drive<WheelGamepadButtonMapping>(gamepad_button(button), profile);
        driven.set_digital(true);
        driven->update_gamecube(report.buf());
    };
    Report<GameCubeGamepad_Data_t> report;
    press(Gamepad_LeftShoulder, report);
    press(Gamepad_A, report);
    EXPECT_EQ(report->leftTrigger, 0xFF);
    EXPECT_EQ(report->rightTrigger, 0);
    EXPECT_FALSE(report->z);
    EXPECT_TRUE(report->a);
}

// Original Xbox racing games expect a Mad Catz MC2's paddles, on A and X
TEST_F(WheelButton, OriginalXboxPaddlesAreAAndX)
{
    auto press = [this](GamepadButtonType button, Report<OGXboxGamepad_Data_t> &report) {
        auto driven = drive<WheelGamepadButtonMapping>(gamepad_button(button), profile);
        driven.set_digital(true);
        driven->update_ogxbox(report.buf());
    };
    Report<OGXboxGamepad_Data_t> report;
    press(Gamepad_LeftShoulder, report);
    press(Gamepad_RightShoulder, report);
    press(Gamepad_Y, report);
    EXPECT_EQ(report->a, 0xFF);
    EXPECT_EQ(report->x, 0xFF);
    EXPECT_EQ(report->y, 0xFF);
    EXPECT_EQ(report->leftShoulder, 0);
    EXPECT_EQ(report->rightShoulder, 0);
}

TEST_F(WheelButton, OtherConsolesHaveNowhereForThem)
{
    Report<XInputGamepad_Data_t> report;
    pressed(Wheel_Gear1)->update_xinput(report.buf());
    Report<XInputGamepad_Data_t> empty;
    EXPECT_EQ(memcmp(report.bytes(), empty.bytes(), sizeof(XInputGamepad_Data_t)), 0);
}

// Xbox One wheels are G920s, with 16 bit steering and 8 bit pedals
TEST_F(WheelAxis, XboxOneUsesTheWheelReport)
{
    Report<XboxOneWheel_Data_t> report;
    axis(Wheel_Steering, 0x1234)->update_xboxone(report.buf());
    axis(Wheel_Accelerator, UINT16_MAX)->update_xboxone(report.buf());
    axis(Wheel_Brake, 0x8000)->update_xboxone(report.buf());
    axis(Wheel_Clutch, 0x0100)->update_xboxone(report.buf());
    axis(Wheel_Handbrake, UINT16_MAX)->update_xboxone(report.buf());
    // payload bytes 2 - 9 (the Linux xbox_gip driver's offsets), little endian
    EXPECT_EQ(report.bytes()[2], 0x34);
    EXPECT_EQ(report.bytes()[3], 0x12);
    EXPECT_EQ(report->accelerator, 0xFF);
    EXPECT_EQ(report->brake, 0x80);
    EXPECT_EQ(report->clutch, 0x01);
    // a G920 has no handbrake
    EXPECT_EQ(report->handbrake, 0);
}

// The tail of a G920's report is the same in every capture (sonik-br's pico_gip_wheel_poc): no
// handbrake, a 6 speed shifter, in neutral, then C8 00 FF F8
TEST(XboxOneWheel, RestsLikeAG920)
{
    XboxOneWheel_Data_t wheel = {};
    xbox_one_wheel_rest(wheel);
    const uint8_t *bytes = (const uint8_t *)&wheel;
    const uint8_t tail[] = {0x00, 0xE6, 0x00, 0xC8, 0x00, 0xFF, 0xF8};
    EXPECT_EQ(memcmp(bytes + 10, tail, sizeof(tail)), 0);
    EXPECT_EQ(wheel.steering, 0x8000);
}

// A gamepad's left stick and triggers on a wheel are the steering and pedals
TEST_F(WheelAxis, GamepadAxesOnAWheelGoInTheWheelReport)
{
    // as the mapping factory makes them, see gamepad_wheel_axis
    auto input = std::make_unique<FakeInput>();
    input->analog = UINT16_MAX;
    WheelAxisMapping mapping(with_gamepad_axis(trigger_config(), Gamepad_RightTrigger), Wheel_Accelerator, std::move(input), 1, profile);
    mapping.update(false, false);
    Report<XboxOneWheel_Data_t> report;
    mapping.update_xboxone(report.buf());
    EXPECT_EQ(report->accelerator, 0xFF);
    // and it still reaches the right trigger where the wheel is a gamepad
    Report<XInputGamepad_Data_t> xinput;
    mapping.update_xinput(xinput.buf());
    EXPECT_EQ(xinput->rightTrigger, 0xFF);
}

TEST_F(WheelAxis, OtherGamepadAxesStayOutOfTheXboxOneWheelReport)
{
    auto driven = drive<WheelGamepadAxisMapping>(with_gamepad_axis(stick_config(), Gamepad_RightStickY), profile);
    driven.set_analog(UINT16_MAX);
    Report<XboxOneWheel_Data_t> report;
    driven->update_xboxone(report.buf());
    Report<XboxOneWheel_Data_t> empty;
    EXPECT_EQ(memcmp(report.bytes(), empty.bytes(), sizeof(XboxOneWheel_Data_t)), 0);
}

// GameCube wheels are Speed Forces: steering in the stick's Y, the pedals on the C stick
TEST_F(WheelAxis, GameCubeUsesTheSpeedForceReport)
{
    Report<GameCubeGamepad_Data_t> report;
    axis(Wheel_Steering, 0x1234)->update_gamecube(report.buf());
    axis(Wheel_Accelerator, UINT16_MAX)->update_gamecube(report.buf());
    axis(Wheel_Brake, 0x8000)->update_gamecube(report.buf());
    axis(Wheel_Clutch, UINT16_MAX)->update_gamecube(report.buf());
    EXPECT_EQ(report->leftStickY, 0x12);
    EXPECT_EQ(report->cStickX, 0xFF);
    EXPECT_EQ(report->cStickY, 0x80);
    // the status byte is left alone, and there's no clutch
    EXPECT_EQ(report->leftStickX, 0);
    EXPECT_EQ(report->leftTrigger, 0);
    EXPECT_EQ(report->rightTrigger, 0);
}

// How USB and bluetooth hosts read a wheel, as no host decodes a wheel's own report yet
TEST(WheelAxisAsGamepad, WheelAxesBecomeTheirGamepadAxis)
{
    proto_Output output = {};
    output.which_mapping = proto_Output_wheelAxis_tag;
    output.mapping.wheelAxis = Wheel_Brake;
    wheel_axis_as_gamepad(output);
    EXPECT_EQ(output.which_mapping, proto_Output_gamepadAxis_tag);
    EXPECT_EQ(output.mapping.gamepadAxis, Gamepad_LeftTrigger);
}

TEST(WheelAxisAsGamepad, OtherOutputsAreLeftAlone)
{
    proto_Output output = gamepad_button(Gamepad_A).mapping;
    wheel_axis_as_gamepad(output);
    EXPECT_EQ(output.which_mapping, proto_Output_gamepadButton_tag);
    EXPECT_EQ(output.mapping.gamepadButton, Gamepad_A);
}
