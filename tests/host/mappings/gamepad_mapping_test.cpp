#include <gtest/gtest.h>
#include "mappings/mapping_test_support.hpp"
#include "devices/usb/host/xinput_tick_helpers.h"
#include "protocols/santroller_v2.hpp"
#include "protocols/face_buttons.hpp"

// How the gamepad mappings pack a calibrated 0 - 65535 axis (stick up / right is high) or a
// button into each console's report
namespace
{
class GamepadAxis : public MappingTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile();

    // A full range stick or trigger, so the raw value is also the calibrated value
    template <typename T = GamepadAxisMapping>
    Driven<T> axis(GamepadAxisType type, uint16_t value)
    {
        bool trigger = type == Gamepad_LeftTrigger || type == Gamepad_RightTrigger;
        auto driven = drive<T>(with_gamepad_axis(trigger ? trigger_config() : stick_config(), type), profile);
        driven.set_analog(value);
        return driven;
    }
};

class GamepadButton : public MappingTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile();

    template <typename T = GamepadButtonMapping>
    Driven<T> pressed(GamepadButtonType type, bool value = true)
    {
        auto driven = drive<T>(gamepad_button(type), profile);
        driven.set_digital(value);
        return driven;
    }

    // An analog button, so the pressure is the analog value
    template <typename T = GamepadButtonMapping>
    Driven<T> pressed_with(GamepadButtonType type, uint16_t pressure)
    {
        auto driven = drive<T>(gamepad_button(type), profile);
        driven.input->independent_analog = true;
        driven.input->analog = pressure;
        driven.set_digital(true);
        return driven;
    }
};
} // namespace

// XInput: signed sticks with up positive, 8 bit triggers

TEST_F(GamepadAxis, XInputSticksAreSignedFullRange)
{
    Report<XInputGamepad_Data_t> report;
    axis(Gamepad_LeftStickX, 0)->update_xinput(report.buf());
    axis(Gamepad_LeftStickY, UINT16_MAX)->update_xinput(report.buf());
    axis(Gamepad_RightStickX, UINT16_MAX)->update_xinput(report.buf());
    axis(Gamepad_RightStickY, 0)->update_xinput(report.buf());
    EXPECT_EQ(report->leftStickX, INT16_MIN);
    EXPECT_EQ(report->leftStickY, INT16_MAX);
    EXPECT_EQ(report->rightStickX, INT16_MAX);
    EXPECT_EQ(report->rightStickY, INT16_MIN);
}

TEST_F(GamepadAxis, XInputStickJustOffCentre)
{
    Report<XInputGamepad_Data_t> report;
    axis(Gamepad_LeftStickX, 32768)->update_xinput(report.buf());
    EXPECT_EQ(report->leftStickX, 0);
}

TEST_F(GamepadAxis, XInputTriggersAreEightBit)
{
    Report<XInputGamepad_Data_t> report;
    axis(Gamepad_LeftTrigger, UINT16_MAX)->update_xinput(report.buf());
    axis(Gamepad_RightTrigger, 0x8000)->update_xinput(report.buf());
    EXPECT_EQ(report->leftTrigger, 0xFF);
    EXPECT_EQ(report->rightTrigger, 0x80);
}

TEST_F(GamepadAxis, CentredAxesLeaveTheReportAlone)
{
    // so another mapping on the same axis can set it
    Report<XInputGamepad_Data_t> report;
    report->leftStickX = 1234;
    report->leftTrigger = 56;
    axis(Gamepad_LeftStickX, UINT16_MAX / 2)->update_xinput(report.buf());
    axis(Gamepad_LeftTrigger, 0)->update_xinput(report.buf());
    EXPECT_EQ(report->leftStickX, 1234);
    EXPECT_EQ(report->leftTrigger, 56);
}

// HID: the same layout as XInput, but HID Y axes are down positive

TEST_F(GamepadAxis, HidFlipsYAxes)
{
    Report<PCGamepadDpad_Data_t> report;
    axis(Gamepad_LeftStickY, UINT16_MAX)->update_hid(report.buf());
    axis(Gamepad_RightStickY, 0)->update_hid(report.buf());
    EXPECT_EQ(report->leftStickY, INT16_MIN);
    EXPECT_EQ(report->rightStickY, INT16_MAX);
}

TEST_F(GamepadAxis, HidKeepsXAxesAndTriggers)
{
    Report<PCGamepadDpad_Data_t> report;
    axis(Gamepad_LeftStickX, UINT16_MAX)->update_hid(report.buf());
    axis(Gamepad_RightStickX, 0)->update_hid(report.buf());
    axis(Gamepad_RightTrigger, UINT16_MAX)->update_hid(report.buf());
    EXPECT_EQ(report->leftStickX, INT16_MAX);
    EXPECT_EQ(report->rightStickX, INT16_MIN);
    EXPECT_EQ(report->rightTrigger, 0xFF);
}

TEST_F(GamepadAxis, HidYCentreStaysCentred)
{
    // 32768 is xinput 0, which flips to -1: one step from centre either way
    Report<PCGamepadDpad_Data_t> report;
    axis(Gamepad_LeftStickY, 32768)->update_hid(report.buf());
    EXPECT_EQ(report->leftStickY, -1);
}

TEST_F(GamepadAxis, Santroller2HostReadsHidSticksBack)
{
    // A Santroller hosting another Santroller 2 reads the HID report with the xinput helpers,
    // so the flipped Y axes have to be flipped back (santroller_host.cpp)
    for (GamepadAxisType type : {Gamepad_LeftStickX, Gamepad_LeftStickY, Gamepad_RightStickX, Gamepad_RightStickY})
    {
        for (uint16_t value : {0, 0x1234, 0x8001, 0xBEEF, UINT16_MAX})
        {
            Report<PCGamepadDpad_Data_t> report;
            axis(type, value)->update_hid(report.buf());
            proto_Output output = {};
            output.which_mapping = proto_Output_gamepadAxis_tag;
            output.mapping.gamepadAxis = type;
            uint16_t read = santroller_v2_fix_analog(output, xinput_tick_analog_impl(report.buf(), Gamepad, output, true), 0);
            EXPECT_EQ(read, value) << "axis " << type << " value " << value;
        }
    }
}

TEST_F(GamepadAxis, HidInstrumentsKeepXInputYAxes)
{
    // instrument reports are XInput reports, so a stick mapped on one isn't flipped
    for (SubType subtype : {GuitarHeroGuitar, RockBandDrums, LiveGuitar, ProKeys, PowerGigGuitar})
    {
        profile = make_profile(subtype);
        Report<PCGamepadDpad_Data_t> report;
        axis(Gamepad_LeftStickY, UINT16_MAX)->update_hid(report.buf());
        axis(Gamepad_RightStickY, 0)->update_hid(report.buf());
        EXPECT_EQ(report->leftStickY, INT16_MAX) << subtype;
        EXPECT_EQ(report->rightStickY, INT16_MIN) << subtype;
    }
    for (SubType subtype : {Gamepad, Dancepad, Taiko, StageKit})
    {
        profile = make_profile(subtype);
        Report<PCGamepadDpad_Data_t> report;
        axis(Gamepad_LeftStickY, UINT16_MAX)->update_hid(report.buf());
        EXPECT_EQ(report->leftStickY, INT16_MIN) << subtype;
    }
}

TEST_F(GamepadAxis, Santroller2HostReadsInstrumentSticksBack)
{
    // instruments report CapabilityXInputYAxis, so the host leaves their Y axes alone
    profile = make_profile(GuitarHeroGuitar);
    for (GamepadAxisType type : {Gamepad_LeftStickX, Gamepad_LeftStickY, Gamepad_RightStickX, Gamepad_RightStickY})
    {
        for (uint16_t value : {0, 0x1234, 0x8001, 0xBEEF, UINT16_MAX})
        {
            Report<PCGamepadDpad_Data_t> report;
            axis(type, value)->update_hid(report.buf());
            proto_Output output = {};
            output.which_mapping = proto_Output_gamepadAxis_tag;
            output.mapping.gamepadAxis = type;
            uint16_t read = santroller_v2_fix_analog(output, xinput_tick_analog_impl(report.buf(), GuitarHeroGuitar, output, true),
                                                     CapabilityXInputYAxis);
            EXPECT_EQ(read, value) << "axis " << type << " value " << value;
        }
    }
}

TEST_F(GamepadAxis, Santroller2HostReadsXInputYAxesBack)
{
    // With "invert Y axis" on, HIDGamepadDevice::process flips a gamepad's Y axes back to
    // XInput's convention, and reports CapabilityXInputYAxis so the host leaves them alone
    for (GamepadAxisType type : {Gamepad_LeftStickX, Gamepad_LeftStickY, Gamepad_RightStickX, Gamepad_RightStickY})
    {
        for (uint16_t value : {0, 0x1234, 0x8001, 0xBEEF, UINT16_MAX})
        {
            Report<PCGamepadDpad_Data_t> report;
            axis(type, value)->update_hid(report.buf());
            report->leftStickY = static_cast<int16_t>(~static_cast<uint16_t>(report->leftStickY));
            report->rightStickY = static_cast<int16_t>(~static_cast<uint16_t>(report->rightStickY));
            proto_Output output = {};
            output.which_mapping = proto_Output_gamepadAxis_tag;
            output.mapping.gamepadAxis = type;
            uint16_t read = santroller_v2_fix_analog(output, xinput_tick_analog_impl(report.buf(), Gamepad, output, true),
                                                     CapabilityXInputYAxis);
            EXPECT_EQ(read, value) << "axis " << type << " value " << value;
        }
    }
}

// PlayStation: unsigned 8 bit sticks centred on 0x80, with up at 0

TEST_F(GamepadAxis, PS3SticksAreEightBitWithUpAtZero)
{
    Report<PS3Dpad_Data_t> report;
    axis(Gamepad_LeftStickX, UINT16_MAX)->update_ps3(report.buf());
    axis(Gamepad_LeftStickY, UINT16_MAX)->update_ps3(report.buf());
    axis(Gamepad_RightStickX, 0)->update_ps3(report.buf());
    axis(Gamepad_RightStickY, 0)->update_ps3(report.buf());
    EXPECT_EQ(report->leftStickX, 0xFF);
    EXPECT_EQ(report->leftStickY, 0x00);
    EXPECT_EQ(report->rightStickX, 0x00);
    EXPECT_EQ(report->rightStickY, 0xFF);
}

TEST_F(GamepadAxis, PS3TriggersSetTheDigitalBitAndPressure)
{
    Report<PS3Dpad_Data_t> report;
    axis(Gamepad_LeftTrigger, UINT16_MAX)->update_ps3(report.buf());
    axis(Gamepad_RightTrigger, 0x4000)->update_ps3(report.buf());
    EXPECT_TRUE(report->l2);
    EXPECT_TRUE(report->r2);
    EXPECT_EQ(report->leftTrigger, 0xFF);
    EXPECT_EQ(report->rightTrigger, 0x40);
}

TEST_F(GamepadAxis, PS3TriggerAtRestLeavesL2Off)
{
    Report<PS3Dpad_Data_t> report;
    axis(Gamepad_LeftTrigger, 0)->update_ps3(report.buf());
    EXPECT_FALSE(report->l2);
    EXPECT_EQ(report->leftTrigger, 0);
}

TEST_F(GamepadAxis, PS3TriggerDigitalInputPressesL2EvenAtRest)
{
    Report<PS3Dpad_Data_t> report;
    auto trigger = axis(Gamepad_LeftTrigger, 0);
    trigger.input->digital = true;
    trigger->update_ps3(report.buf());
    EXPECT_TRUE(report->l2);
}

// The PS3 motion axes are 10 bit and rest at 0x200 (see PS3Dpad_Data_t in protocols/ps3.hpp
// and PS3_ACCEL_CENTER)
TEST_F(GamepadAxis, PS3AccelIsTenBit)
{
    Report<PS3Dpad_Data_t> report;
    axis(Gamepad_AccelX, UINT16_MAX)->update_ps3(report.buf());
    axis(Gamepad_Gyro, 0)->update_ps3(report.buf());
    EXPECT_EQ(report->accelX, 0x3FF);
    EXPECT_EQ(report->gyro, 0);
}

TEST_F(GamepadAxis, PS4SticksAndTriggers)
{
    Report<PS4Gamepad_Data_t> report;
    axis(Gamepad_LeftStickX, 0)->update_ps4(report.buf());
    axis(Gamepad_LeftStickY, 0)->update_ps4(report.buf());
    axis(Gamepad_RightStickY, UINT16_MAX)->update_ps4(report.buf());
    axis(Gamepad_LeftTrigger, UINT16_MAX)->update_ps4(report.buf());
    EXPECT_EQ(report->leftStickX, 0x00);
    EXPECT_EQ(report->leftStickY, 0xFF);
    EXPECT_EQ(report->rightStickY, 0x00);
    EXPECT_EQ(report->leftTrigger, 0xFF);
}

TEST_F(GamepadAxis, PS5SticksAndTriggers)
{
    Report<PS5Gamepad_Data_t> report;
    axis(Gamepad_RightStickX, UINT16_MAX)->update_ps5(report.buf());
    axis(Gamepad_LeftStickY, UINT16_MAX)->update_ps5(report.buf());
    axis(Gamepad_RightTrigger, 0x8000)->update_ps5(report.buf());
    EXPECT_EQ(report->rightStickX, 0xFF);
    EXPECT_EQ(report->leftStickY, 0x00);
    EXPECT_EQ(report->rightTrigger, 0x80);
}

TEST_F(GamepadAxis, PS2SticksAndTriggers)
{
    Report<PS2Gamepad_Data_t> report;
    axis(Gamepad_LeftStickX, UINT16_MAX)->update_ps2(report.buf());
    axis(Gamepad_LeftStickY, UINT16_MAX)->update_ps2(report.buf());
    axis(Gamepad_RightTrigger, UINT16_MAX)->update_ps2(report.buf());
    EXPECT_EQ(report->leftStickX, 0xFF);
    EXPECT_EQ(report->leftStickY, 0x00);
    EXPECT_TRUE(report->r2);
    EXPECT_EQ(report->rightTrigger, 0xFF);
}

TEST_F(GamepadAxis, PS3DualShockSticks)
{
    Report<PS3Gamepad_Data_t> report;
    axis<PS3GamepadAxisMapping>(Gamepad_LeftStickX, UINT16_MAX)->update_ps3(report.buf());
    axis<PS3GamepadAxisMapping>(Gamepad_LeftStickY, UINT16_MAX)->update_ps3(report.buf());
    axis<PS3GamepadAxisMapping>(Gamepad_LeftTrigger, UINT16_MAX)->update_ps3(report.buf());
    EXPECT_EQ(report->leftStickX, 0xFF);
    EXPECT_EQ(report->leftStickY, 0x00);
    EXPECT_TRUE(report->l2);
    EXPECT_EQ(report->leftTrigger, 0xFF);
}

// Xbox One: signed sticks, 10 bit triggers

TEST_F(GamepadAxis, XboxOneTriggersAreTenBit)
{
    Report<XboxOneGamepad_Data_t> report;
    axis(Gamepad_LeftTrigger, UINT16_MAX)->update_xboxone(report.buf());
    axis(Gamepad_RightTrigger, 0x8000)->update_xboxone(report.buf());
    EXPECT_EQ(report->leftTrigger, 0x3FF);
    EXPECT_EQ(report->rightTrigger, 0x200);
}

TEST_F(GamepadAxis, XboxOneSticksAreSigned)
{
    Report<XboxOneGamepad_Data_t> report;
    axis(Gamepad_LeftStickX, 0)->update_xboxone(report.buf());
    axis(Gamepad_RightStickY, UINT16_MAX)->update_xboxone(report.buf());
    EXPECT_EQ(report->leftStickX, INT16_MIN);
    EXPECT_EQ(report->rightStickY, INT16_MAX);
}

TEST_F(GamepadAxis, OGXboxSticksAndTriggers)
{
    Report<OGXboxGamepad_Data_t> report;
    axis(Gamepad_LeftStickY, UINT16_MAX)->update_ogxbox(report.buf());
    axis(Gamepad_RightStickX, 0)->update_ogxbox(report.buf());
    axis(Gamepad_LeftTrigger, UINT16_MAX)->update_ogxbox(report.buf());
    EXPECT_EQ(report->leftStickY, INT16_MAX);
    EXPECT_EQ(report->rightStickX, INT16_MIN);
    EXPECT_EQ(report->leftTrigger, 0xFF);
}

// Switch: 12 bit sticks with up high, digital ZL / ZR

TEST_F(GamepadAxis, SwitchSticksMatchAdvertisedCalibration)
{
    Report<SwitchInputReport> report;
    axis(Gamepad_LeftStickX, UINT16_MAX)->update_switch(report.buf());
    axis(Gamepad_LeftStickY, UINT16_MAX)->update_switch(report.buf());
    axis(Gamepad_RightStickX, 0)->update_switch(report.buf());
    axis(Gamepad_RightStickY, 0x8000)->update_switch(report.buf());
    EXPECT_EQ(report->leftStickX, 0xEA4);
    EXPECT_EQ(report->leftStickY, 0xEA4);
    EXPECT_EQ(report->rightStickX, 0x15C);
    EXPECT_EQ(report->rightStickY, 0x800);
    EXPECT_EQ(report.bytes()[4], 0xA4);
    EXPECT_EQ(report.bytes()[5], 0x4E);
    EXPECT_EQ(report.bytes()[6], 0xEA);
    EXPECT_EQ(report.bytes()[7], 0x5C);
    EXPECT_EQ(report.bytes()[8], 0x01);
    EXPECT_EQ(report.bytes()[9], 0x80);
}

TEST_F(GamepadAxis, SwitchStickRangeIsMonotonicAndBounded)
{
    uint16_t previous = SWITCH_STICK_CENTER - SWITCH_STICK_TRAVEL;
    for (uint32_t value = 0; value <= UINT16_MAX; value++)
    {
        uint16_t stick = switch_stick(value);
        ASSERT_GE(stick, previous) << value;
        ASSERT_LE(stick, SWITCH_STICK_CENTER + SWITCH_STICK_TRAVEL) << value;
        previous = stick;
    }
    EXPECT_EQ(switch_stick(UINT16_MAX / 2), SWITCH_STICK_CENTER);
    EXPECT_EQ(switch_stick(0x8000), SWITCH_STICK_CENTER);
}

TEST_F(GamepadAxis, SwitchTriggersPressNearTheEnd)
{
    Report<SwitchInputReport> report;
    axis(Gamepad_LeftTrigger, 60000)->update_switch(report.buf());
    EXPECT_FALSE(report->leftTrigger);
    axis(Gamepad_LeftTrigger, 60001)->update_switch(report.buf());
    EXPECT_TRUE(report->leftTrigger);
    axis(Gamepad_RightTrigger, UINT16_MAX)->update_switch(report.buf());
    EXPECT_TRUE(report->rightTrigger);
}

// Wii classic controller, in each of its data formats

TEST_F(GamepadAxis, WiiFormat3IsEightBit)
{
    Report<WiiClassicDataFormat3_t> report;
    axis(Gamepad_LeftStickX, UINT16_MAX)->update_wii(3, report.buf());
    axis(Gamepad_RightStickY, 0x8000)->update_wii(3, report.buf());
    axis(Gamepad_LeftTrigger, UINT16_MAX)->update_wii(3, report.buf());
    EXPECT_EQ(report->leftStickX, 0xFF);
    EXPECT_EQ(report->rightStickY, 0x80);
    EXPECT_EQ(report->leftTrigger, 0xFF);
    EXPECT_TRUE(report->l2);
}

TEST_F(GamepadAxis, WiiFormat2HasTenBitSticksSplitAcrossBytes)
{
    // format 2: bytes 0-3 are bits 9-2 of LX, RX, LY, RY, byte 4 their bits 1-0
    Report<WiiClassicDataFormat2_t> report;
    axis(Gamepad_LeftStickX, UINT16_MAX)->update_wii(2, report.buf());
    axis(Gamepad_RightStickY, 0x0040)->update_wii(2, report.buf()); // 10 bit value 1
    EXPECT_EQ(report.bytes()[0], 0xFF);
    EXPECT_EQ(report.bytes()[3], 0x00);
    EXPECT_EQ(report.bytes()[4], 0b01000011);
}

TEST_F(GamepadAxis, WiiFormat2TriggersAreEightBit)
{
    Report<WiiClassicDataFormat2_t> report;
    axis(Gamepad_RightTrigger, UINT16_MAX)->update_wii(2, report.buf());
    EXPECT_EQ(report->rightTrigger, 0xFF);
    EXPECT_TRUE(report->r2);
}

TEST_F(GamepadAxis, WiiFormat1LeftStickIsSixBit)
{
    // format 1: byte 0 bits 5-0 are LX, byte 1 bits 5-0 are LY
    Report<WiiClassicDataFormat1_t> report;
    axis(Gamepad_LeftStickX, UINT16_MAX)->update_wii(1, report.buf());
    axis(Gamepad_LeftStickY, 0x8000)->update_wii(1, report.buf());
    EXPECT_EQ(report.bytes()[0], 0x3F);
    EXPECT_EQ(report.bytes()[1], 0x20);
}

TEST_F(GamepadAxis, WiiFormat1RightStickXIsSplitOverThreeBytes)
{
    // RX is 5 bits: bits 4-3 in byte 0 bits 7-6, bits 2-1 in byte 1 bits 7-6, bit 0 in byte 2 bit 7
    Report<WiiClassicDataFormat1_t> report;
    axis(Gamepad_RightStickX, UINT16_MAX)->update_wii(1, report.buf());
    EXPECT_EQ(report.bytes()[0], 0xC0);
    EXPECT_EQ(report.bytes()[1], 0xC0);
    EXPECT_EQ(report.bytes()[2], 0x80);
}

TEST_F(GamepadAxis, WiiFormat1RightStickYAndTriggers)
{
    // byte 2 bits 4-0 are RY and bits 6-5 LT bits 4-3, byte 3 bits 7-5 are LT bits 2-0 and bits 4-0 RT
    Report<WiiClassicDataFormat1_t> report;
    axis(Gamepad_RightStickY, UINT16_MAX)->update_wii(1, report.buf());
    EXPECT_EQ(report.bytes()[2], 0x1F);
    Report<WiiClassicDataFormat1_t> triggers;
    axis(Gamepad_LeftTrigger, UINT16_MAX)->update_wii(1, triggers.buf());
    EXPECT_EQ(triggers.bytes()[2], 0x60);
    EXPECT_EQ(triggers.bytes()[3] & 0xE0, 0xE0);
    Report<WiiClassicDataFormat1_t> right;
    axis(Gamepad_RightTrigger, UINT16_MAX)->update_wii(1, right.buf());
    EXPECT_EQ(right.bytes()[3] & 0x1F, 0x1F);
}

// Drum and turntable reports keep instrument data where the sticks go

TEST_F(GamepadAxis, DrumsDropSticksButKeepTriggers)
{
    Report<XInputGamepad_Data_t> report;
    axis<DrumsGamepadAxisMapping>(Gamepad_LeftStickX, UINT16_MAX)->update_xinput(report.buf());
    axis<DrumsGamepadAxisMapping>(Gamepad_LeftTrigger, UINT16_MAX)->update_xinput(report.buf());
    EXPECT_EQ(report->leftStickX, 0);
    EXPECT_EQ(report->leftTrigger, 0xFF);
}

TEST_F(GamepadAxis, DrumsStillSendSticksToPlayStation)
{
    Report<PS3Dpad_Data_t> report;
    axis<DrumsGamepadAxisMapping>(Gamepad_LeftStickX, UINT16_MAX)->update_ps3(report.buf());
    EXPECT_EQ(report->leftStickX, 0xFF);
}

TEST_F(GamepadAxis, TurntableDropsAllAxesOnXbox)
{
    Report<XInputGamepad_Data_t> report;
    axis<DJHTurntableGamepadAxisMapping>(Gamepad_LeftStickX, UINT16_MAX)->update_xinput(report.buf());
    axis<DJHTurntableGamepadAxisMapping>(Gamepad_LeftTrigger, UINT16_MAX)->update_xinput(report.buf());
    axis<DJHTurntableGamepadAxisMapping>(Gamepad_LeftTrigger, UINT16_MAX)->update_hid(report.buf());
    EXPECT_EQ(report->leftStickX, 0);
    EXPECT_EQ(report->leftTrigger, 0);
}

// Buttons

TEST_F(GamepadButton, XInputButtons)
{
    Report<XInputGamepad_Data_t> report;
    pressed(Gamepad_A)->update_xinput(report.buf());
    pressed(Gamepad_Guide)->update_xinput(report.buf());
    pressed(Gamepad_DpadLeft)->update_xinput(report.buf());
    pressed(Gamepad_B, false)->update_xinput(report.buf());
    EXPECT_TRUE(report->a);
    EXPECT_TRUE(report->guide);
    EXPECT_TRUE(report->dpadLeft);
    EXPECT_FALSE(report->b);
}

TEST_F(GamepadButton, ReleasedButtonDoesNotClearAnotherMappingsPress)
{
    Report<XInputGamepad_Data_t> report;
    pressed(Gamepad_A)->update_xinput(report.buf());
    pressed(Gamepad_A, false)->update_xinput(report.buf());
    EXPECT_TRUE(report->a);
}

TEST_F(GamepadButton, HidUsesTheXInputLayout)
{
    Report<PCGamepadDpad_Data_t> report;
    pressed(Gamepad_Y)->update_hid(report.buf());
    pressed(Gamepad_Capture)->update_hid(report.buf());
    EXPECT_TRUE(report->y);
    EXPECT_TRUE(report->capture);
}

TEST_F(GamepadButton, PS2PressureIsTheHighestOfItsMappings)
{
    Report<PS2Gamepad_Data_t> report;
    pressed_with(Gamepad_A, 0x4000)->update_ps2(report.buf());
    pressed_with(Gamepad_A, 0xC000)->update_ps2(report.buf());
    pressed_with(Gamepad_A, 0x2000)->update_ps2(report.buf());
    EXPECT_TRUE(report->a);
    EXPECT_EQ(report->pressureCross, 0xC0);
}

TEST_F(GamepadButton, PS2DigitalPressIsFullPressure)
{
    Report<PS2Gamepad_Data_t> report;
    pressed(Gamepad_DpadUp)->update_ps2(report.buf());
    pressed(Gamepad_RightShoulder)->update_ps2(report.buf());
    EXPECT_EQ(report->pressureDpadUp, 0xFF);
    EXPECT_EQ(report->pressureR1, 0xFF);
    EXPECT_EQ(report->pressureDpadDown, 0);
}

TEST_F(GamepadButton, PS3DualShockPressure)
{
    Report<PS3Gamepad_Data_t> report;
    pressed_with<PS3GamepadButtonMapping>(Gamepad_X, 0x8000)->update_ps3(report.buf());
    pressed<PS3GamepadButtonMapping>(Gamepad_LeftShoulder)->update_ps3(report.buf());
    EXPECT_TRUE(report->x);
    EXPECT_EQ(report->pressureSquare, 0x80);
    EXPECT_EQ(report->pressureL1, 0xFF);
}

TEST_F(GamepadButton, PS3ThirdPartyButtons)
{
    Report<PS3ThirdPartyGamepad_Data_t> report;
    pressed(Gamepad_Guide)->update_ps3(report.buf());
    pressed(Gamepad_DpadRight)->update_ps3(report.buf());
    EXPECT_TRUE(report->guide);
    EXPECT_TRUE(report->dpadRight);
}

TEST_F(GamepadButton, OGXboxFaceButtonsAreAnalog)
{
    Report<OGXboxGamepad_Data_t> report;
    pressed_with(Gamepad_A, 0x8000)->update_ogxbox(report.buf());
    pressed(Gamepad_LeftShoulder)->update_ogxbox(report.buf());
    pressed(Gamepad_Start)->update_ogxbox(report.buf());
    EXPECT_EQ(report->a, 0x80);
    EXPECT_EQ(report->leftShoulder, 0xFF);
    EXPECT_EQ(report->b, 0);
    EXPECT_TRUE(report->start);
}

TEST_F(GamepadButton, OGXboxAnalogButtonKeepsTheHighest)
{
    Report<OGXboxGamepad_Data_t> report;
    pressed_with(Gamepad_Y, 0xF000)->update_ogxbox(report.buf());
    pressed_with(Gamepad_Y, 0x1000)->update_ogxbox(report.buf());
    EXPECT_EQ(report->y, 0xF0);
}

TEST_F(GamepadButton, SelectIsBackOnAGamepad)
{
    profile->select_to_dpad_left = true;
    Report<PS4Gamepad_Data_t> report;
    pressed(Gamepad_Back)->update_ps4(report.buf());
    EXPECT_TRUE(report->back);
    EXPECT_FALSE(report->dpadLeft);
}

TEST_F(GamepadButton, SelectCanBeDpadLeftOnGuitars)
{
    profile->subtype = GuitarHeroGuitar;
    profile->select_to_dpad_left = true;
    Report<PS4Gamepad_Data_t> ps4;
    pressed(Gamepad_Back)->update_ps4(ps4.buf());
    EXPECT_FALSE(ps4->back);
    EXPECT_TRUE(ps4->dpadLeft);

    Report<PS5Gamepad_Data_t> ps5;
    pressed(Gamepad_Back)->update_ps5(ps5.buf());
    EXPECT_FALSE(ps5->back);
    EXPECT_TRUE(ps5->dpadLeft);

    Report<XboxOneGamepad_Data_t> xb1;
    pressed(Gamepad_Back)->update_xboxone(xb1.buf());
    EXPECT_FALSE(xb1->back);
    EXPECT_TRUE(xb1->dpadLeft);

    Report<SwitchInputReport> nx;
    pressed(Gamepad_Back)->update_switch(nx.buf());
    EXPECT_FALSE(nx->back);
    EXPECT_TRUE(nx->dpadLeft);
}

TEST_F(GamepadButton, PS5CaptureIsTheTouchpad)
{
    Report<PS5Gamepad_Data_t> report;
    pressed(Gamepad_Capture)->update_ps5(report.buf());
    EXPECT_TRUE(report->touchpad);
}

TEST_F(GamepadButton, XboxOneCaptureIsTheShareConsoleFunction)
{
    Report<XboxOneGamepad_Data_t> report;
    pressed(Gamepad_Capture)->update_xboxone(report.buf());
    EXPECT_EQ(report->consoleFunctions[0], 0x01);
}

TEST_F(GamepadButton, XboxOneCaptureFollowsTheDrumPayload)
{
    profile->subtype = RockBandDrums;
    Report<XboxOneRockBandDrums_Data_t> report;
    pressed(Gamepad_Capture)->update_xboxone(report.buf());
    EXPECT_EQ(report->consoleFunctions[0], 0x01);
}

TEST_F(GamepadButton, XboxOneCaptureReleasedSetsNothing)
{
    Report<XboxOneGamepad_Data_t> report;
    pressed(Gamepad_Capture, false)->update_xboxone(report.buf());
    EXPECT_EQ(report->consoleFunctions[0], 0);
}

TEST_F(GamepadButton, SwitchButtons)
{
    Report<SwitchInputReport> report;
    pressed(Gamepad_Capture)->update_switch(report.buf());
    pressed(Gamepad_LeftThumbClick)->update_switch(report.buf());
    pressed(Gamepad_DpadDown)->update_switch(report.buf());
    EXPECT_TRUE(report->capture);
    EXPECT_TRUE(report->leftThumbClick);
    EXPECT_TRUE(report->dpadDown);
}

// Legend based: the mapped legend is pressed, wherever it sits on the console's pad
TEST_F(GamepadButton, SwitchFaceButtonsGoByLegend)
{
    Report<SwitchInputReport> report;
    pressed(Gamepad_A)->update_switch(report.buf());
    pressed(Gamepad_X)->update_switch(report.buf());
    EXPECT_TRUE(report->a);
    EXPECT_TRUE(report->x);
    EXPECT_FALSE(report->b);
    EXPECT_FALSE(report->y);
}

// Position based: A is the bottom button, which is B on a Switch pad, and X the left one, which is Y
TEST_F(GamepadButton, SwitchFaceButtonsGoByPositionInPositionMode)
{
    profile->position_based_face_buttons = true;
    Report<SwitchInputReport> report;
    pressed(Gamepad_A)->update_switch(report.buf());
    pressed(Gamepad_X)->update_switch(report.buf());
    EXPECT_TRUE(report->b);
    EXPECT_TRUE(report->y);
    EXPECT_FALSE(report->a);
    EXPECT_FALSE(report->x);
    pressed(Gamepad_B)->update_switch(report.buf());
    pressed(Gamepad_Y)->update_switch(report.buf());
    EXPECT_TRUE(report->a);
    EXPECT_TRUE(report->x);
}

TEST_F(GamepadButton, XboxLayoutsAreTheSameInPositionMode)
{
    profile->position_based_face_buttons = true;
    Report<XInputGamepad_Data_t> report;
    pressed(Gamepad_A)->update_xinput(report.buf());
    pressed(Gamepad_X)->update_xinput(report.buf());
    EXPECT_TRUE(report->a);
    EXPECT_TRUE(report->x);
    EXPECT_FALSE(report->b);
    EXPECT_FALSE(report->y);
}

TEST_F(GamepadButton, WiiClassicFaceButtonsGoByPositionInPositionMode)
{
    profile->position_based_face_buttons = true;
    Report<WiiClassicDataFormat3_t> report;
    pressed(Gamepad_A)->update_wii(3, report.buf());
    EXPECT_TRUE(report->b);
    EXPECT_FALSE(report->a);
}

// A Nintendo pad being read in position based mode: the bottom position reads its B, and so on
TEST(FaceButtons, NintendoPadsSwapBothPairs)
{
    proto_Output output = {};
    output.which_mapping = proto_Output_gamepadButton_tag;
    const struct
    {
        GamepadButtonType position;
        GamepadButtonType legend;
    } expected[] = {
        {Gamepad_A, Gamepad_B},
        {Gamepad_B, Gamepad_A},
        {Gamepad_X, Gamepad_Y},
        {Gamepad_Y, Gamepad_X},
        {Gamepad_Start, Gamepad_Start},
        {Gamepad_DpadUp, Gamepad_DpadUp},
    };
    for (const auto &c : expected)
    {
        output.mapping.gamepadButton = c.position;
        EXPECT_EQ(swap_nintendo_face_button(output).mapping.gamepadButton, c.legend) << int(c.position);
    }
}

TEST(FaceButtons, OnlyGamepadButtonsAreSwapped)
{
    proto_Output output = {};
    output.which_mapping = proto_Output_gamepadAxis_tag;
    output.mapping.gamepadAxis = Gamepad_LeftStickX;
    EXPECT_EQ(swap_nintendo_face_button(output).mapping.gamepadAxis, Gamepad_LeftStickX);
}

TEST_F(GamepadButton, WiiClassicButtonsInEachFormat)
{
    Report<WiiClassicDataFormat1_t> f1;
    Report<WiiClassicDataFormat2_t> f2;
    Report<WiiClassicDataFormat3_t> f3;
    auto a = pressed(Gamepad_A);
    a->update_wii(1, f1.buf());
    a->update_wii(2, f2.buf());
    a->update_wii(3, f3.buf());
    EXPECT_TRUE(f1->a);
    EXPECT_TRUE(f2->a);
    EXPECT_TRUE(f3->a);
}

TEST_F(GamepadButton, WiimoteCoreButtons)
{
    wiimote_buttons buttons;
    memset(&buttons, 0, sizeof(buttons));
    pressed(Gamepad_Back)->update_wiimote_core(&buttons);
    pressed(Gamepad_Start)->update_wiimote_core(&buttons);
    pressed(Gamepad_Guide)->update_wiimote_core(&buttons);
    EXPECT_TRUE(buttons.minus);
    EXPECT_TRUE(buttons.plus);
    EXPECT_TRUE(buttons.home);
    EXPECT_FALSE(buttons.a);
}

TEST_F(GamepadAxis, DualShockAccelIsTenBitBigEndian)
{
    // the DS3 sends its motion axes big endian, like the device's own PS3_ACCEL_CENTER default
    Report<PS3Gamepad_Data_t> report;
    axis<PS3GamepadAxisMapping>(Gamepad_AccelY, UINT16_MAX)->update_ps3(report.buf());
    axis<PS3GamepadAxisMapping>(Gamepad_Gyro, 0x8000)->update_ps3(report.buf());
    EXPECT_EQ(report->accelY, __builtin_bswap16(0x3FF));
    EXPECT_EQ(report->gyro, __builtin_bswap16(0x200));
}

TEST_F(GamepadAxis, PS3AccelAtRestKeepsTheDefault)
{
    Report<PS3Dpad_Data_t> report;
    report->accelZ = PS3_ACCEL_CENTER;
    axis(Gamepad_AccelZ, UINT16_MAX / 2)->update_ps3(report.buf());
    EXPECT_EQ(report->accelZ, PS3_ACCEL_CENTER);
}

// GameCube, N64, SNES and NES: the expected bytes are what goes out on the wire, MSB first

TEST_F(GamepadAxis, GameCubeSticksAreEightBitWithUpHigh)
{
    Report<GameCubeGamepad_Data_t> report;
    axis(Gamepad_LeftStickX, 0)->update_gamecube(report.buf());
    axis(Gamepad_LeftStickY, UINT16_MAX)->update_gamecube(report.buf());
    axis(Gamepad_RightStickX, UINT16_MAX)->update_gamecube(report.buf());
    axis(Gamepad_RightStickY, 0)->update_gamecube(report.buf());
    EXPECT_EQ(report.bytes()[2], 0x00);
    EXPECT_EQ(report.bytes()[3], 0xFF);
    EXPECT_EQ(report.bytes()[4], 0xFF);
    EXPECT_EQ(report.bytes()[5], 0x00);
}

TEST_F(GamepadAxis, GameCubeTriggersClickAtTheEndOfTheirTravel)
{
    Report<GameCubeGamepad_Data_t> report;
    axis(Gamepad_LeftTrigger, 0x8000)->update_gamecube(report.buf());
    axis(Gamepad_RightTrigger, UINT16_MAX)->update_gamecube(report.buf());
    EXPECT_EQ(report.bytes()[6], 0x80);
    EXPECT_EQ(report.bytes()[7], 0xFF);
    // L is 0x40 and R is 0x20 in the second byte
    EXPECT_EQ(report.bytes()[1], 0x20);
}

TEST_F(GamepadAxis, N64StickIsSignedAndLimitedToARealSticksRange)
{
    Report<N64Gamepad_Data_t> report;
    axis(Gamepad_LeftStickX, 0)->update_n64(report.buf());
    axis(Gamepad_LeftStickY, UINT16_MAX)->update_n64(report.buf());
    EXPECT_EQ((int8_t)report.bytes()[2], -N64_STICK_RANGE);
    EXPECT_EQ((int8_t)report.bytes()[3], N64_STICK_RANGE);
}

TEST_F(GamepadAxis, N64RightStickPressesCButtons)
{
    Report<N64Gamepad_Data_t> report;
    axis(Gamepad_RightStickX, UINT16_MAX)->update_n64(report.buf());
    axis(Gamepad_RightStickY, 0)->update_n64(report.buf());
    // C right is 0x01 and C down 0x04
    EXPECT_EQ(report.bytes()[1], 0x05);
}

TEST_F(GamepadAxis, N64TriggersAreZ)
{
    Report<N64Gamepad_Data_t> report;
    axis(Gamepad_LeftTrigger, 0x8000)->update_n64(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x00);
    axis(Gamepad_RightTrigger, UINT16_MAX)->update_n64(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x20);
}

TEST_F(GamepadButton, GameCubeFaceButtonsGoByLegend)
{
    Report<GameCubeGamepad_Data_t> report;
    pressed(Gamepad_A)->update_gamecube(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x01);
    pressed(Gamepad_X)->update_gamecube(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x05);
    pressed(Gamepad_B)->update_gamecube(report.buf());
    pressed(Gamepad_Y)->update_gamecube(report.buf());
    pressed(Gamepad_Start)->update_gamecube(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x1F);
}

TEST_F(GamepadButton, GameCubeFaceButtonsGoByPositionInPositionMode)
{
    profile->position_based_face_buttons = true;
    Report<GameCubeGamepad_Data_t> report;
    pressed(Gamepad_A)->update_gamecube(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x01);
    // X is the left button, which is B on a GameCube pad
    pressed(Gamepad_X)->update_gamecube(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x03);
    pressed(Gamepad_B)->update_gamecube(report.buf());
    pressed(Gamepad_Y)->update_gamecube(report.buf());
    pressed(Gamepad_Start)->update_gamecube(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x1F);
}

TEST_F(GamepadButton, GameCubeDpadAndZ)
{
    Report<GameCubeGamepad_Data_t> report;
    pressed(Gamepad_DpadUp)->update_gamecube(report.buf());
    pressed(Gamepad_DpadLeft)->update_gamecube(report.buf());
    pressed(Gamepad_RightShoulder)->update_gamecube(report.buf());
    EXPECT_EQ(report.bytes()[1], 0x19);
}

TEST_F(GamepadButton, N64Buttons)
{
    Report<N64Gamepad_Data_t> report;
    pressed(Gamepad_A)->update_n64(report.buf());
    pressed(Gamepad_X)->update_n64(report.buf());
    pressed(Gamepad_Start)->update_n64(report.buf());
    pressed(Gamepad_DpadRight)->update_n64(report.buf());
    EXPECT_EQ(report.bytes()[0], 0xD1);
    pressed(Gamepad_LeftShoulder)->update_n64(report.buf());
    pressed(Gamepad_RightShoulder)->update_n64(report.buf());
    EXPECT_EQ(report.bytes()[1], 0x30);
}

TEST_F(GamepadButton, SnesFaceButtonsGoByLegend)
{
    Report<SNESGamepad_Data_t> report;
    // A is in the second byte, B is the first bit clocked out
    pressed(Gamepad_A)->update_snes(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x00);
    EXPECT_EQ(report.bytes()[1], 0x01);
    pressed(Gamepad_B)->update_snes(report.buf());
    pressed(Gamepad_Y)->update_snes(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x03);
    pressed(Gamepad_X)->update_snes(report.buf());
    EXPECT_EQ(report.bytes()[1], 0x03);
}

TEST_F(GamepadButton, SnesFaceButtonsGoByPositionInPositionMode)
{
    profile->position_based_face_buttons = true;
    Report<SNESGamepad_Data_t> report;
    // A is the bottom button, which is B on a SNES pad and the first bit clocked out
    pressed(Gamepad_A)->update_snes(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x01);
    EXPECT_EQ(report.bytes()[1], 0x00);
    pressed(Gamepad_B)->update_snes(report.buf());
    pressed(Gamepad_Y)->update_snes(report.buf());
    pressed(Gamepad_RightShoulder)->update_snes(report.buf());
    EXPECT_EQ(report.bytes()[1], 0x0B);
    pressed(Gamepad_X)->update_snes(report.buf());
    pressed(Gamepad_Back)->update_snes(report.buf());
    pressed(Gamepad_DpadRight)->update_snes(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x87);
}

TEST_F(GamepadButton, NesButtonsGoByLegend)
{
    Report<NESGamepad_Data_t> report;
    pressed(Gamepad_A)->update_nes(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x01);
    pressed(Gamepad_B)->update_nes(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x03);
}

TEST_F(GamepadButton, NesButtonsGoByPositionInPositionMode)
{
    profile->position_based_face_buttons = true;
    Report<NESGamepad_Data_t> report;
    // B (the right button) is A on a NES pad, the first bit clocked out
    pressed(Gamepad_B)->update_nes(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x01);
    pressed(Gamepad_A)->update_nes(report.buf());
    pressed(Gamepad_Start)->update_nes(report.buf());
    pressed(Gamepad_DpadUp)->update_nes(report.buf());
    EXPECT_EQ(report.bytes()[0], 0x1B);
}

// The dpad bits (up 1, down 2, left 4, right 8) become a HID hat: 0 is up, going clockwise to 7, and 8 is
// neutral. The nibble can hold any combination, and opposite directions held together cancel out.
TEST(GamepadDpadHat, EveryCombinationHasAHatValue)
{
    constexpr uint8_t UP = 1, DOWN = 2, LEFT = 4, RIGHT = 8;
    const struct
    {
        uint8_t bits;
        uint8_t hat;
    } expected[] = {
        {0, 8},
        {UP, 0},
        {UP | RIGHT, 1},
        {RIGHT, 2},
        {DOWN | RIGHT, 3},
        {DOWN, 4},
        {DOWN | LEFT, 5},
        {LEFT, 6},
        {UP | LEFT, 7},
        {UP | DOWN, 8},
        {LEFT | RIGHT, 8},
        {UP | DOWN | LEFT, 8},
        {UP | DOWN | RIGHT, 8},
        {UP | LEFT | RIGHT, 8},
        {DOWN | LEFT | RIGHT, 8},
        {UP | DOWN | LEFT | RIGHT, 8},
    };
    static_assert(sizeof(GamepadButtonMapping::dpad_bindings) == 16);
    for (const auto &c : expected)
    {
        EXPECT_EQ(GamepadButtonMapping::dpad_bindings[c.bits], c.hat) << int(c.bits);
        // a Santroller receiving this over bluetooth turns the hat back into the bits
        uint8_t report[3] = {1, 0, c.hat};
        santroller_v2_normalize_report(report, sizeof(report), GuitarHeroGuitar);
        EXPECT_EQ(report[2], c.hat == 8 ? 0 : c.bits) << int(c.bits);
    }
}
