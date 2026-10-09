// Toggle and cycling inputs. A toggle device flips its state on each press of a toggle input
// ("Toggle Input", with its "Current State" shown in the tool). A cycling device jumps through a
// list of values, one step per press: "each time you press the button, it jumps between each
// notch" (cycle.description), forwards and / or backwards ("Allow cycling forwards / backwards").
// Both act on the press edge only, ignore a second press within 100 ms (switch bounce), tell the
// tool about each change and save it to the aux config so it survives a reboot.
#include "inputs_test_support.hpp"
#include "input/toggle.hpp"
#include "input/cycle.hpp"
#include "devices/toggle.hpp"
#include "devices/cycle.hpp"

namespace
{
std::shared_ptr<ToggleDevice> toggle_device(uint16_t id, bool current, const DeviceReloadState *state = nullptr)
{
    proto_ToggleDevice config = proto_ToggleDevice_init_default;
    return std::make_shared<ToggleDevice>(state, config, id, current);
}

std::shared_ptr<CycleDevice> cycle_device(uint16_t id, uint32_t index, std::vector<uint32_t> values,
                                          const DeviceReloadState *state = nullptr)
{
    proto_CycleDevice config = proto_CycleDevice_init_default;
    config.type = CycleType_custom;
    return std::make_shared<CycleDevice>(state, config, id, index, values);
}

std::vector<proto_Event> events_of(pb_size_t tag)
{
    std::vector<proto_Event> found;
    for (const auto &event : HIDConfigDevice::sent_events)
    {
        if (event.which_event == tag)
        {
            found.push_back(event);
        }
    }
    return found;
}

class ToggleTest : public InputsTest
{
protected:
    std::shared_ptr<ToggleDevice> device = toggle_device(3, false);
    ToggleInput toggle;
    TestInput *button = nullptr;
    void SetUp() override
    {
        InputsTest::SetUp();
        proto_ToggleInput config = proto_ToggleInput_init_default;
        config.deviceid = 3;
        toggle.load(config, device, test_input(button, pin_id(7)));
    }
    // A press long enough to get past the bounce filter, then a release
    void tap()
    {
        button->set(true);
        toggle.tick_digital();
        clock_ms::advance(150);
        button->set(false);
        toggle.tick_digital();
        clock_ms::advance(150);
    }
};

class CycleTest : public InputsTest
{
protected:
    std::shared_ptr<CycleDevice> device = cycle_device(5, 0, {10, 20, 30});
    CycleInput cycle;
    TestInput *forward = nullptr;
    TestInput *backward = nullptr;
    void SetUp() override
    {
        InputsTest::SetUp();
        proto_CycleInput config = proto_CycleInput_init_default;
        config.deviceid = 5;
        cycle.load(config, device, test_input(forward, pin_id(7)), test_input(backward, pin_id(8)));
    }
    void tap(TestInput *button)
    {
        button->set(true);
        cycle.tick_analog();
        clock_ms::advance(150);
        button->set(false);
        cycle.tick_analog();
        clock_ms::advance(150);
    }
};
} // namespace

TEST_F(ToggleTest, StartsAtTheSavedState)
{
    EXPECT_FALSE(toggle.tick_digital());
    auto on = toggle_device(3, true);
    ToggleInput other;
    other.load(proto_ToggleInput_init_default, on, std::make_unique<TestInput>());
    EXPECT_TRUE(other.tick_digital());
    EXPECT_EQ(other.tick_analog(), UINT16_MAX);
}

TEST_F(ToggleTest, EachPressFlipsTheState)
{
    tap();
    EXPECT_TRUE(toggle.tick_digital());
    EXPECT_EQ(toggle.tick_analog(), UINT16_MAX);
    tap();
    EXPECT_FALSE(toggle.tick_digital());
    EXPECT_EQ(toggle.tick_analog(), 0);
    tap();
    EXPECT_TRUE(toggle.tick_digital());
}

TEST_F(ToggleTest, HoldingTheButtonFlipsOnlyOnce)
{
    button->set(true);
    for (int i = 0; i < 1000; i++)
    {
        EXPECT_TRUE(toggle.tick_digital());
        clock_ms::advance(1);
    }
    // Releasing doesn't flip it either
    button->set(false);
    EXPECT_TRUE(toggle.tick_digital());
    clock_ms::advance(1000);
    EXPECT_TRUE(toggle.tick_digital());
}

TEST_F(ToggleTest, ButtonAlreadyHeldWhenLoadedCountsAsAPress)
{
    // Edge detection starts from "released", so a button already held when the toggle loads
    // counts as a press
    button->set(true);
    EXPECT_TRUE(toggle.tick_digital());
}

TEST_F(ToggleTest, BounceWithin100MsIsIgnored)
{
    button->set(true);
    toggle.tick_digital();
    ASSERT_TRUE(device->get_value());
    // The switch bounces open and closed again
    clock_ms::advance(20);
    button->set(false);
    toggle.tick_digital();
    clock_ms::advance(20);
    button->set(true);
    EXPECT_TRUE(toggle.tick_digital());
    clock_ms::advance(59);
    button->set(false);
    toggle.tick_digital();
    button->set(true);
    // 99 ms after the first press: still a bounce
    EXPECT_TRUE(toggle.tick_digital());
    button->set(false);
    toggle.tick_digital();
    clock_ms::advance(1);
    button->set(true);
    // 100 ms after: a real press
    EXPECT_FALSE(toggle.tick_digital());
}

TEST_F(ToggleTest, ChangesAreReportedAndSaved)
{
    tap();
    tap();
    auto events = events_of(proto_Event_toggle_tag);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].event.toggle.id, 3u);
    EXPECT_TRUE(events[0].event.toggle.state);
    EXPECT_FALSE(events[1].event.toggle.state);
    ASSERT_EQ(fake_config::aux_toggles.size(), 2u);
    EXPECT_EQ(fake_config::aux_toggles[0], std::make_pair(3u, true));
    EXPECT_EQ(fake_config::aux_toggles[1], std::make_pair(3u, false));
}

TEST_F(ToggleTest, InputsSharingADeviceSeeOneToggle)
{
    // Two mappings using the same toggle device both see the press, but it only flips once
    ToggleInput second;
    TestInput *second_button = nullptr;
    second.load(proto_ToggleInput_init_default, device, test_input(second_button));
    button->set(true);
    second_button->set(true);
    EXPECT_TRUE(toggle.tick_digital());
    EXPECT_TRUE(second.tick_digital());
    EXPECT_EQ(events_of(proto_Event_toggle_tag).size(), 1u);
}

TEST_F(ToggleTest, StateAndBounceSurviveAReload)
{
    tap();
    ASSERT_TRUE(device->get_value());
    // Press again, then reload before the bounce window is over
    button->set(true);
    toggle.tick_digital();
    ASSERT_FALSE(device->get_value());
    DeviceReloadState saved;
    device->save_reload_state(saved);
    EXPECT_TRUE(saved.valid);
    auto reloaded = toggle_device(3, true, &saved);
    EXPECT_FALSE(reloaded->get_value());
    clock_ms::advance(50);
    reloaded->toggle();
    EXPECT_FALSE(reloaded->get_value());
    clock_ms::advance(50);
    reloaded->toggle();
    EXPECT_TRUE(reloaded->get_value());
}

TEST_F(ToggleTest, PassesThroughTheInnerInput)
{
    EXPECT_EQ(toggle.hardware_id(), pin_id(7));
    EXPECT_TRUE(toggle.valid());
    button->is_valid = false;
    EXPECT_FALSE(toggle.valid());
    toggle.setup();
    EXPECT_EQ(button->setups, 1);
}

TEST_F(ToggleTest, WithoutAnInputItJustReportsTheDevice)
{
    ToggleInput bare;
    bare.load(proto_ToggleInput_init_default, toggle_device(4, true), nullptr);
    EXPECT_TRUE(bare.tick_digital());
    EXPECT_FALSE(bare.valid());
}

TEST_F(ToggleTest, WithoutAnInputSetupDoesNotCrash)
{
    // proto ToggleInput.input is optional and config.cpp loads a missing one as nullptr.
    // Mapping::reload() (run after the tool's pin detection) calls setup(), which has to skip the
    // missing input like tick_digital does. The configurator always fills the input in, so this
    // needs a hand made or imported config.
    EXPECT_EXIT(
        {
            ToggleInput bare;
            bare.load(proto_ToggleInput_init_default, toggle_device(4, true), nullptr);
            bare.setup();
            exit(0);
        },
        ::testing::ExitedWithCode(0), "");
}

TEST_F(CycleTest, ForwardStepsThroughTheValuesAndWraps)
{
    EXPECT_EQ(cycle.tick_analog(), 10);
    tap(forward);
    EXPECT_EQ(cycle.tick_analog(), 20);
    tap(forward);
    EXPECT_EQ(cycle.tick_analog(), 30);
    tap(forward);
    EXPECT_EQ(cycle.tick_analog(), 10);
}

TEST_F(CycleTest, BackwardStepsBackAndWrapsToTheLastValue)
{
    tap(backward);
    EXPECT_EQ(cycle.tick_analog(), 30);
    tap(backward);
    EXPECT_EQ(cycle.tick_analog(), 20);
    tap(forward);
    EXPECT_EQ(cycle.tick_analog(), 30);
}

TEST_F(CycleTest, HoldingStepsOnce)
{
    forward->set(true);
    for (int i = 0; i < 1000; i++)
    {
        EXPECT_EQ(cycle.tick_analog(), 20);
        clock_ms::advance(1);
    }
}

TEST_F(CycleTest, BounceWithin100MsIsIgnored)
{
    forward->set(true);
    cycle.tick_analog();
    clock_ms::advance(30);
    forward->set(false);
    cycle.tick_analog();
    clock_ms::advance(30);
    forward->set(true);
    EXPECT_EQ(cycle.tick_analog(), 20);
    // The bounce filter is shared by both directions
    backward->set(true);
    EXPECT_EQ(cycle.tick_analog(), 20);
}

TEST_F(CycleTest, StartsAtTheSavedIndex)
{
    auto device = cycle_device(6, 2, {10, 20, 30});
    CycleInput other;
    other.load(proto_CycleInput_init_default, device, std::make_unique<TestInput>(), nullptr);
    EXPECT_EQ(other.tick_analog(), 30);
}

TEST_F(CycleTest, ChangesReportTheIndexAndAreSaved)
{
    tap(forward);
    tap(backward);
    tap(backward);
    auto events = events_of(proto_Event_cycle_tag);
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].event.cycle.id, 5u);
    EXPECT_EQ(events[0].event.cycle.state, 1u);
    EXPECT_EQ(events[1].event.cycle.state, 0u);
    EXPECT_EQ(events[2].event.cycle.state, 2u);
    ASSERT_EQ(fake_config::aux_cycles.size(), 3u);
    EXPECT_EQ(fake_config::aux_cycles[2], std::make_pair(5u, 2u));
}

TEST_F(CycleTest, DigitalIsWhetherTheValueIsNonZero)
{
    auto device = cycle_device(6, 0, {0, 0x1900});
    CycleInput other;
    TestInput *button = nullptr;
    other.load(proto_CycleInput_init_default, device, test_input(button), nullptr);
    EXPECT_FALSE(other.tick_digital());
    button->set(true);
    EXPECT_TRUE(other.tick_digital());
}

TEST_F(CycleTest, PickupSelectorNotches)
{
    // The configurator's pickup selector values
    auto device = cycle_device(6, 0, {0x1900, 0x4c00, 0x9600, 0xb200, 0xe500});
    CycleInput pickup;
    TestInput *button = nullptr;
    pickup.load(proto_CycleInput_init_default, device, test_input(button), nullptr);
    const uint16_t expected[] = {0x4c00, 0x9600, 0xb200, 0xe500, 0x1900};
    for (uint16_t value : expected)
    {
        button->set(true);
        pickup.tick_analog();
        clock_ms::advance(100);
        button->set(false);
        pickup.tick_analog();
        clock_ms::advance(100);
        EXPECT_EQ(pickup.tick_analog(), value);
    }
}

TEST_F(CycleTest, SingleValueStaysPut)
{
    auto device = cycle_device(6, 0, {7});
    CycleInput single;
    TestInput *button = nullptr;
    single.load(proto_CycleInput_init_default, device, test_input(button), nullptr);
    button->set(true);
    EXPECT_EQ(single.tick_analog(), 7);
}

TEST_F(CycleTest, PressesRightAfterTheDeviceStartsAreIgnored)
{
    // begin() starts the bounce window, so a press in the first 100 ms after loading is dropped
    device->begin();
    clock_ms::advance(99);
    forward->set(true);
    EXPECT_EQ(cycle.tick_analog(), 10);
    forward->set(false);
    cycle.tick_analog();
    clock_ms::advance(1);
    forward->set(true);
    EXPECT_EQ(cycle.tick_analog(), 20);
}

TEST_F(CycleTest, BackwardOnlyCycleWorks)
{
    // "Allow cycling forwards" turned off leaves only the reverse input
    auto device = cycle_device(6, 0, {1, 2, 3});
    CycleInput reverse_only;
    TestInput *button = nullptr;
    reverse_only.load(proto_CycleInput_init_default, device, nullptr, test_input(button));
    button->set(true);
    EXPECT_EQ(reverse_only.tick_analog(), 3);
}

TEST_F(CycleTest, SetupRestoresTheReverseInput)
{
    // Mapping::reload() runs setup() after the tool's pin detection, which resets the pins it
    // watched (gpio_init / no pulls), so setup() has to restore both the forward and the
    // backwards button or the backwards one floats until the next reboot.
    cycle.setup();
    EXPECT_EQ(forward->setups, 1);
    EXPECT_EQ(backward->setups, 1);
}

TEST_F(CycleTest, BackwardOnlyCycleSetupDoesNotCrash)
{
    // Turning "Allow cycling forwards" off in the configurator (Inputs.page.tsx sets cycle.input
    // to null) loads a null forward input (config.cpp), so setup(), run by Mapping::reload()
    // after pin detection, must only set up the inputs that exist.
    EXPECT_EXIT(
        {
            auto device = cycle_device(6, 0, {1, 2, 3});
            CycleInput reverse_only;
            reverse_only.load(proto_CycleInput_init_default, device, nullptr, std::make_unique<TestInput>());
            reverse_only.setup();
            exit(0);
        },
        ::testing::ExitedWithCode(0), "");
}

TEST_F(CycleTest, BackwardOnlyCycleIsValid)
{
    // Without a forward input valid() and hardware_id() fall back to the reverse input, so a
    // backwards only cycle isn't treated as invalid (an axis on it would always be "centered")
    // and can still be masked by a shortcut.
    auto device = cycle_device(6, 0, {1, 2, 3});
    CycleInput reverse_only;
    reverse_only.load(proto_CycleInput_init_default, device, nullptr, std::make_unique<TestInput>(pin_id(8)));
    EXPECT_TRUE(reverse_only.valid());
    EXPECT_EQ(reverse_only.hardware_id(), pin_id(8));
}

TEST_F(CycleTest, EmptyValueListDoesNotCrash)
{
    // The configurator's custom cycle values are a TagsInput (Devices.page.tsx) that can be
    // emptied, and a saved index can be past the end after values are removed (device_factory.cpp
    // passes the saved index straight through). CycleDevice falls back to index 0 (value 0 for an
    // empty list) rather than reading out of bounds.
    EXPECT_EXIT(
        {
            auto empty = cycle_device(6, 0, {});
            auto shrunk = cycle_device(7, 4, {1, 2});
            exit(empty && shrunk ? 0 : 1);
        },
        ::testing::ExitedWithCode(0), "");
}
