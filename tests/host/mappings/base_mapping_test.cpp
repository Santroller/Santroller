#include <gtest/gtest.h>
#include "mappings/mapping_test_support.hpp"
#include "managers/config_manager.hpp"
#include "pico/bootrom.h"

// AxisMapping::update and ButtonMapping::update: how an input sample becomes the value the
// console outputs read (m_calibrated_value / m_centered for axes, m_last_value /
// m_last_pressure for buttons)
namespace
{
constexpr uint32_t kStickCentre = UINT16_MAX / 2;

class ProbeAxis : public GamepadAxisMapping
{
public:
    using GamepadAxisMapping::GamepadAxisMapping;
    uint32_t value() const { return m_calibrated_value; }
    bool centered() const { return m_centered; }
};

class ProbeButton : public GamepadButtonMapping
{
public:
    using GamepadButtonMapping::GamepadButtonMapping;
    bool value() const { return m_last_value; }
    uint16_t pressure() const { return m_last_pressure; }
};

class AxisUpdate : public MappingTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile();
};

class ButtonUpdate : public MappingTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile();
};
} // namespace

TEST_F(AxisUpdate, StickAtRestIsCentred)
{
    auto axis = drive<ProbeAxis>(with_gamepad_axis(stick_config(), Gamepad_LeftStickX), profile);
    axis.set_analog(kStickCentre);
    EXPECT_EQ(axis->value(), kStickCentre);
    EXPECT_TRUE(axis->centered());
}

TEST_F(AxisUpdate, StickMovedIsNotCentred)
{
    auto axis = drive<ProbeAxis>(with_gamepad_axis(stick_config(), Gamepad_LeftStickX), profile);
    axis.set_analog(UINT16_MAX);
    EXPECT_EQ(axis->value(), UINT16_MAX);
    EXPECT_FALSE(axis->centered());
    axis.set_analog(0);
    EXPECT_EQ(axis->value(), 0u);
    EXPECT_FALSE(axis->centered());
}

TEST_F(AxisUpdate, TriggerAtRestIsCentred)
{
    auto axis = drive<ProbeAxis>(with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger), profile);
    axis.set_analog(0);
    EXPECT_EQ(axis->value(), 0u);
    EXPECT_TRUE(axis->centered());
    axis.set_analog(UINT16_MAX);
    EXPECT_EQ(axis->value(), UINT16_MAX);
    EXPECT_FALSE(axis->centered());
}

TEST_F(AxisUpdate, TriggerAxesAreCalibratedAsTriggers)
{
    // a trigger's deadzone is measured up from min, not around a centre
    auto config = with_gamepad_axis(trigger_config(), Gamepad_RightTrigger);
    config.deadzone = 1000;
    auto axis = drive<ProbeAxis>(config, profile);
    axis.set_analog(999);
    EXPECT_EQ(axis->value(), 0u);
    EXPECT_TRUE(axis->centered());
}

TEST_F(AxisUpdate, InvalidInputCountsAsCentred)
{
    auto axis = drive<ProbeAxis>(with_gamepad_axis(stick_config(), Gamepad_LeftStickX), profile);
    axis.input->is_valid = false;
    axis.set_analog(UINT16_MAX);
    EXPECT_TRUE(axis->centered());
}

TEST_F(AxisUpdate, DigitalToAxisUsesPressedValue)
{
    auto config = with_gamepad_axis(stick_config(), Gamepad_LeftStickX);
    config.has_pressed = true;
    config.pressed = 1000;
    config.has_released = true;
    config.released = 50000;
    auto axis = drive<ProbeAxis>(config, profile);
    axis.set_digital(true);
    EXPECT_EQ(axis->value(), 1000u);
    axis.set_digital(false);
    EXPECT_EQ(axis->value(), 50000u);
}

TEST_F(AxisUpdate, DigitalToAxisWithoutReleasedValueRestsAtCentre)
{
    auto config = with_gamepad_axis(stick_config(), Gamepad_LeftStickX);
    config.has_pressed = true;
    config.pressed = UINT16_MAX;
    auto axis = drive<ProbeAxis>(config, profile);
    axis.set_digital(true);
    EXPECT_EQ(axis->value(), UINT16_MAX);
    EXPECT_FALSE(axis->centered());
    axis.set_digital(false);
    EXPECT_EQ(axis->value(), (uint32_t)config.center);
    EXPECT_TRUE(axis->centered());
}

TEST_F(AxisUpdate, PositiveHalfPushesFromCentreToTop)
{
    auto config = with_gamepad_axis(trigger_config(), Gamepad_LeftStickX);
    config.has_section = true;
    config.section = AxisSectionPositive;
    auto axis = drive<ProbeAxis>(config, profile);
    axis.set_analog(0);
    EXPECT_EQ(axis->value(), kStickCentre);
    EXPECT_TRUE(axis->centered());
    axis.set_analog(UINT16_MAX);
    EXPECT_NEAR(axis->value(), UINT16_MAX, 1);
    EXPECT_FALSE(axis->centered());
    axis.set_analog(32768);
    EXPECT_NEAR(axis->value(), kStickCentre + 16384, 1);
}

TEST_F(AxisUpdate, NegativeHalfPushesFromCentreToBottom)
{
    auto config = with_gamepad_axis(trigger_config(), Gamepad_LeftStickX);
    config.has_section = true;
    config.section = AxisSectionNegative;
    auto axis = drive<ProbeAxis>(config, profile);
    axis.set_analog(0);
    EXPECT_EQ(axis->value(), kStickCentre);
    EXPECT_TRUE(axis->centered());
    axis.set_analog(UINT16_MAX);
    EXPECT_EQ(axis->value(), 0u);
    EXPECT_FALSE(axis->centered());
}

TEST_F(AxisUpdate, HalfAxisIgnoresTheConfiguredCentre)
{
    // a half axis rests at the axis centre whatever its raw centre is
    auto config = with_gamepad_axis(trigger_config(), Gamepad_LeftStickY);
    config.has_section = true;
    config.section = AxisSectionPositive;
    config.center = 1234;
    auto axis = drive<ProbeAxis>(config, profile);
    axis.set_analog(0);
    EXPECT_TRUE(axis->centered());
}

TEST_F(AxisUpdate, DebounceHoldsTheLastValueBeforeReturningToRest)
{
    auto config = with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger);
    config.has_debounce = true;
    config.debounce = 5; // ms
    auto axis = drive<ProbeAxis>(config, profile);
    axis.set_analog(40000);
    EXPECT_EQ(axis->value(), 40000u);

    fake_time::advance_us(4000);
    axis.set_analog(0);
    EXPECT_EQ(axis->value(), 40000u);
    EXPECT_FALSE(axis->centered());

    fake_time::advance_us(1001);
    axis.set_analog(0);
    EXPECT_EQ(axis->value(), 0u);
    EXPECT_TRUE(axis->centered());
}

TEST_F(AxisUpdate, DebounceInHundredMicrosecondsTakesPriority)
{
    auto config = with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger);
    config.has_debounce = true;
    config.debounce = 50; // ms, ignored
    config.has_debounce100us = true;
    config.debounce100us = 3; // 300us
    auto axis = drive<ProbeAxis>(config, profile);
    axis.set_analog(40000);
    fake_time::advance_us(300);
    axis.set_analog(0);
    EXPECT_EQ(axis->value(), 40000u);
    fake_time::advance_us(1);
    axis.set_analog(0);
    EXPECT_EQ(axis->value(), 0u);
}

TEST_F(AxisUpdate, WithoutDebounceFollowsTheInput)
{
    auto axis = drive<ProbeAxis>(with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger), profile);
    axis.set_analog(40000);
    axis.set_analog(10000);
    EXPECT_EQ(axis->value(), 10000u);
    axis.set_analog(0);
    EXPECT_EQ(axis->value(), 0u);
}

TEST_F(AxisUpdate, PeakBasedKeepsTheHighestValueUntilRest)
{
    auto config = with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger);
    config.has_peakBased = true;
    config.peakBased = true;
    auto axis = drive<ProbeAxis>(config, profile);
    axis.set_analog(30000);
    axis.set_analog(50000);
    axis.set_analog(20000);
    EXPECT_EQ(axis->value(), 50000u);
    axis.set_analog(0);
    EXPECT_EQ(axis->value(), 0u);
    axis.set_analog(10000);
    EXPECT_EQ(axis->value(), 10000u);
}

// An explicitly disabled peakBased behaves like no peakBased at all
TEST_F(AxisUpdate, PeakBasedExplicitlyFalseFollowsTheInput)
{
    auto config = with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger);
    config.has_peakBased = true;
    config.peakBased = false;
    auto axis = drive<ProbeAxis>(config, profile);
    axis.set_analog(50000);
    axis.set_analog(20000);
    EXPECT_EQ(axis->value(), 20000u);
}

TEST_F(AxisUpdate, ShortcutMaskHoldsAtRestUntilReleased)
{
    auto axis = drive<ProbeAxis>(with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger), profile);
    axis->mask_by_shortcut();
    axis.set_analog(UINT16_MAX);
    EXPECT_EQ(axis->value(), 0u);
    EXPECT_TRUE(axis->centered());
    // still held: the mask lasts until the input goes back to rest
    axis.set_analog(UINT16_MAX);
    EXPECT_EQ(axis->value(), 0u);
    axis.set_analog(0);
    axis.set_analog(UINT16_MAX);
    EXPECT_EQ(axis->value(), UINT16_MAX);
}

TEST_F(AxisUpdate, SendsCalibratedValueToConfigTool)
{
    auto axis = drive<ProbeAxis>(with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger), profile);
    axis.input->analog = 40000;
    axis->update(false, true);
    ASSERT_EQ(HIDConfigDevice::sent_events.size(), 1u);
    const auto &event = HIDConfigDevice::sent_events[0];
    EXPECT_EQ(event.which_event, proto_Event_axis_tag);
    // unchanged values aren't sent again unless it is a full poll
    axis->update(false, true);
    EXPECT_EQ(HIDConfigDevice::sent_events.size(), 1u);
    axis->update(true, true);
    EXPECT_EQ(HIDConfigDevice::sent_events.size(), 2u);
}

TEST_F(ButtonUpdate, DigitalPressIsFullPressure)
{
    auto button = drive<ProbeButton>(gamepad_button(Gamepad_A), profile);
    button.set_digital(true);
    EXPECT_TRUE(button->value());
    EXPECT_EQ(button->pressure(), UINT16_MAX);
    button.set_digital(false);
    EXPECT_FALSE(button->value());
    EXPECT_EQ(button->pressure(), 0);
}

TEST_F(ButtonUpdate, InvertedDigital)
{
    auto config = gamepad_button(Gamepad_A);
    config.inverted = true;
    auto button = drive<ProbeButton>(config, profile);
    button.set_digital(false);
    EXPECT_TRUE(button->value());
    EXPECT_EQ(button->pressure(), UINT16_MAX);
    button.set_digital(true);
    EXPECT_FALSE(button->value());
    EXPECT_EQ(button->pressure(), 0);
}

TEST_F(ButtonUpdate, AnalogThresholdPressureIsTheAnalogValue)
{
    auto config = gamepad_button(Gamepad_A);
    config.has_trigger = true;
    config.trigger = AnalogToDigitalTriggerType_JoyHigh;
    config.triggerValue = 40000;
    auto button = drive<ProbeButton>(config, profile);
    button.set_analog(40000);
    EXPECT_FALSE(button->value());
    EXPECT_EQ(button->pressure(), 0);
    button.set_analog(50000);
    EXPECT_TRUE(button->value());
    EXPECT_EQ(button->pressure(), 50000);
}

TEST_F(ButtonUpdate, AnalogThresholdIgnoresTheDigitalReading)
{
    auto config = gamepad_button(Gamepad_A);
    config.has_trigger = true;
    config.trigger = AnalogToDigitalTriggerType_JoyLow;
    config.triggerValue = 10000;
    auto button = drive<ProbeButton>(config, profile);
    button.input->digital = true;
    button.set_analog(20000);
    EXPECT_FALSE(button->value());
    button.set_analog(5000);
    EXPECT_TRUE(button->value());
}

TEST_F(ButtonUpdate, InvertedAnalogThreshold)
{
    auto config = gamepad_button(Gamepad_A);
    config.has_trigger = true;
    config.trigger = AnalogToDigitalTriggerType_JoyHigh;
    config.triggerValue = 40000;
    config.inverted = true;
    auto button = drive<ProbeButton>(config, profile);
    button.set_analog(10000);
    EXPECT_TRUE(button->value());
    button.set_analog(50000);
    EXPECT_FALSE(button->value());
}

TEST_F(ButtonUpdate, RangeThreshold)
{
    auto config = gamepad_button(Gamepad_DpadUp);
    config.has_trigger = true;
    config.trigger = AnalogToDigitalTriggerType_Range;
    config.triggerValue = 20000;
    config.maxTriggerValue = 30000;
    auto button = drive<ProbeButton>(config, profile);
    button.set_analog(25000);
    EXPECT_TRUE(button->value());
    button.set_analog(30000);
    EXPECT_FALSE(button->value());
}

TEST_F(ButtonUpdate, IndependentAnalogInputSetsPressure)
{
    auto button = drive<ProbeButton>(gamepad_button(Gamepad_A), profile);
    button.input->independent_analog = true;
    button.input->analog = 12345;
    button.set_digital(true);
    EXPECT_TRUE(button->value());
    EXPECT_EQ(button->pressure(), 12345);
}

TEST_F(ButtonUpdate, DebounceHoldsAPressForTheDebounceTime)
{
    auto config = gamepad_button(Gamepad_A);
    config.has_debounce = true;
    config.debounce = 10; // ms
    auto button = drive<ProbeButton>(config, profile);
    button.set_digital(true);
    fake_time::advance_us(10000);
    button.set_digital(false);
    EXPECT_TRUE(button->value());
    fake_time::advance_us(1);
    button.set_digital(false);
    EXPECT_FALSE(button->value());
}

TEST_F(ButtonUpdate, DebounceDoesNotDelayAPress)
{
    auto config = gamepad_button(Gamepad_A);
    config.has_debounce = true;
    config.debounce = 10;
    auto button = drive<ProbeButton>(config, profile);
    button.set_digital(true);
    EXPECT_TRUE(button->value());
}

TEST_F(ButtonUpdate, ShortcutMaskHoldsReleasedUntilLetGo)
{
    auto button = drive<ProbeButton>(gamepad_button(Gamepad_A), profile);
    button->mask_by_shortcut();
    button.set_digital(true);
    EXPECT_FALSE(button->value());
    button.set_digital(true);
    EXPECT_FALSE(button->value());
    button.set_digital(false);
    button.set_digital(true);
    EXPECT_TRUE(button->value());
}

TEST_F(ButtonUpdate, MaskedMappingsAreSuppressedWhileThisIsHeld)
{
    auto shortcut = drive<ProbeButton>(gamepad_button(Gamepad_Guide), profile);
    auto masked = drive<ProbeButton>(gamepad_button(Gamepad_A), profile);
    shortcut->add_masked_mapping(masked.mapping.get());
    masked.input->digital = true;
    shortcut.set_digital(true);
    masked->update(false, false);
    EXPECT_FALSE(masked->value());
    EXPECT_TRUE(shortcut->value());
}

TEST_F(ButtonUpdate, CombinedStrumDebounceBlocksTheOtherDirection)
{
    profile->combined_strum_debounce = true;
    auto up = drive<ProbeButton>(gamepad_button(Gamepad_DpadUp), profile);
    auto down = drive<ProbeButton>(gamepad_button(Gamepad_DpadDown), profile);
    up->set_strum_bit(0);
    down->set_strum_bit(1);
    profile->strum_mappings = {up.mapping.get(), down.mapping.get()};
    up.set_digital(true);
    down.set_digital(true);
    EXPECT_TRUE(up->value());
    EXPECT_FALSE(down->value());
    up.set_digital(false);
    down.set_digital(true);
    EXPECT_TRUE(down->value());
}

TEST_F(ButtonUpdate, SendsButtonEventsOnChange)
{
    auto button = drive<ProbeButton>(gamepad_button(Gamepad_A), profile);
    button.input->digital = true;
    button->update(false, true);
    button->update(false, true);
    ASSERT_EQ(HIDConfigDevice::sent_events.size(), 1u);
    EXPECT_EQ(HIDConfigDevice::sent_events[0].which_event, proto_Event_button_tag);
    button.input->digital = false;
    button->update(false, true);
    EXPECT_EQ(HIDConfigDevice::sent_events.size(), 2u);
}

// Action mappings run on the controller itself, once per press

namespace
{
proto_Mapping action(ActionType type)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_action_tag;
    config.mapping.mapping.action = type;
    return config;
}
} // namespace

TEST_F(ButtonUpdate, ActionRunsOnceWhenPressed)
{
    ConfigManager::instance().take_device_stack_restart();
    auto input = std::make_unique<FakeInput>();
    FakeInput *raw = input.get();
    ActionMapping restart(action(ActionRestartDeviceStack), std::move(input), 1, profile);
    restart.update_action();
    EXPECT_FALSE(ConfigManager::instance().take_device_stack_restart());
    raw->digital = true;
    restart.update_action();
    EXPECT_TRUE(ConfigManager::instance().take_device_stack_restart());
    restart.update_action();
    EXPECT_FALSE(ConfigManager::instance().take_device_stack_restart());
}

TEST_F(ButtonUpdate, ActionHeldAtStartNeedsReleasingFirst)
{
    // so a button still held after a restart doesn't restart again
    ConfigManager::instance().take_device_stack_restart();
    auto input = std::make_unique<FakeInput>();
    FakeInput *raw = input.get();
    raw->digital = true;
    ActionMapping restart(action(ActionRestartDeviceStack), std::move(input), 1, profile);
    restart.update_action();
    EXPECT_FALSE(ConfigManager::instance().take_device_stack_restart());
    raw->digital = false;
    restart.update_action();
    raw->digital = true;
    restart.update_action();
    EXPECT_TRUE(ConfigManager::instance().take_device_stack_restart());
}

TEST_F(ButtonUpdate, BootloaderAction)
{
    int before = fake_bootrom::reset_count;
    auto input = std::make_unique<FakeInput>();
    FakeInput *raw = input.get();
    ActionMapping bootloader(action(ActionBootloader), std::move(input), 1, profile);
    bootloader.update_action();
    raw->digital = true;
    bootloader.update_action();
    EXPECT_EQ(fake_bootrom::reset_count, before + 1);
}

// Configs from the config tool centre sticks on a raw 32768, but a calibrated stick rests on
// 32767 (UINT16_MAX / 2). Rest has to be judged on the calibrated value, or the stick never
// counts as centred.

namespace
{
proto_Mapping stick_centred_on(GamepadAxisType axis, int32_t center, int32_t deadzone = 0)
{
    proto_Mapping config = with_gamepad_axis(stick_config(), axis);
    config.center = center;
    config.deadzone = deadzone;
    return config;
}
} // namespace

TEST_F(AxisUpdate, StickCentredOnTheRawMidpointIsCentredAtRest)
{
    auto axis = drive<ProbeAxis>(stick_centred_on(Gamepad_LeftStickX, 32768), profile);
    axis.set_analog(32768);
    EXPECT_EQ(axis->value(), kStickCentre);
    EXPECT_TRUE(axis->centered());
}

TEST_F(AxisUpdate, StickInsideItsDeadzoneIsCentred)
{
    auto axis = drive<ProbeAxis>(stick_centred_on(Gamepad_LeftStickX, 32768, 2000), profile);
    axis.set_analog(33500);
    EXPECT_TRUE(axis->centered());
}

TEST_F(AxisUpdate, RestingStickLeavesTheAxisToAnotherMapping)
{
    // e.g. a dpad mapped onto the stick, alongside the stick itself
    auto dpad_config = with_gamepad_axis(stick_config(), Gamepad_LeftStickX);
    dpad_config.has_pressed = true;
    dpad_config.pressed = UINT16_MAX;
    auto dpad = drive<ProbeAxis>(dpad_config, profile);
    auto stick = drive<ProbeAxis>(stick_centred_on(Gamepad_LeftStickX, 32768), profile);
    dpad.set_digital(true);
    stick.set_analog(32768);
    Report<XInputGamepad_Data_t> report;
    dpad->update_xinput(report.buf());
    stick->update_xinput(report.buf());
    EXPECT_EQ(report->leftStickX, INT16_MAX);
}

TEST_F(AxisUpdate, ShortcutMaskOnAStickEndsBackAtCentre)
{
    auto axis = drive<ProbeAxis>(stick_centred_on(Gamepad_LeftStickX, 32768), profile);
    axis->mask_by_shortcut();
    axis.set_analog(UINT16_MAX);
    EXPECT_EQ(axis->value(), kStickCentre);
    axis.set_analog(32768);
    axis.set_analog(UINT16_MAX);
    EXPECT_EQ(axis->value(), UINT16_MAX);
}
