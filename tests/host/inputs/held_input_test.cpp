// HeldInput: the configurator's "Held Input" with a "Time (ms)" - the wrapped input only counts
// once it has been held down for that long, and a release starts the wait again. Actions use it
// so "a stray press does nothing" (inputs.action_hold_hint), 1000 ms to restart the device stack
// and 3000 ms for the bootloader.
#include "inputs_test_support.hpp"
#include "input/held.hpp"
#include "input/shortcut.hpp"

namespace
{
proto_HeldInput held_for(int32_t ms)
{
    proto_HeldInput config = proto_HeldInput_init_default;
    config.time = ms;
    return config;
}

class HeldInputTest : public InputsTest
{
protected:
    HeldInput held;
    TestInput *button = nullptr;
    void load(int32_t ms) { held.load(held_for(ms), test_input(button, pin_id(4))); }
};
} // namespace

TEST_F(HeldInputTest, NotPressedUntilHeldForTheTime)
{
    load(1000);
    button->set(true);
    EXPECT_FALSE(held.tick_digital());
    clock_ms::advance(500);
    EXPECT_FALSE(held.tick_digital());
    clock_ms::advance(500);
    // Exactly the time is not yet enough, the firmware wants longer than it
    EXPECT_FALSE(held.tick_digital());
    clock_ms::advance(1);
    EXPECT_TRUE(held.tick_digital());
    EXPECT_EQ(held.tick_analog(), UINT16_MAX);
}

TEST_F(HeldInputTest, StaysPressedWhileHeldAndReleasesImmediately)
{
    load(200);
    button->set(true);
    held.tick_digital();
    clock_ms::advance(201);
    EXPECT_TRUE(held.tick_digital());
    clock_ms::advance(5000);
    EXPECT_TRUE(held.tick_digital());
    button->set(false);
    EXPECT_FALSE(held.tick_digital());
    EXPECT_EQ(held.tick_analog(), 0);
}

TEST_F(HeldInputTest, ReleaseRestartsTheWait)
{
    load(1000);
    button->set(true);
    held.tick_digital();
    clock_ms::advance(900);
    EXPECT_FALSE(held.tick_digital());
    // A short release, then pressing again needs the full time from the new press
    button->set(false);
    EXPECT_FALSE(held.tick_digital());
    clock_ms::advance(10);
    button->set(true);
    EXPECT_FALSE(held.tick_digital());
    clock_ms::advance(900);
    EXPECT_FALSE(held.tick_digital());
    clock_ms::advance(101);
    EXPECT_TRUE(held.tick_digital());
}

TEST_F(HeldInputTest, PressNotSampledDuringHoldStillCountsFromFirstSample)
{
    // The wait starts when the press is first seen, not when it physically started
    load(100);
    button->set(true);
    clock_ms::advance(1000);
    EXPECT_FALSE(held.tick_digital());
    clock_ms::advance(101);
    EXPECT_TRUE(held.tick_digital());
}

TEST_F(HeldInputTest, NoInnerInputIsNeverPressed)
{
    held.load(held_for(0), nullptr);
    EXPECT_FALSE(held.tick_digital());
    EXPECT_EQ(held.tick_analog(), 0);
    EXPECT_FALSE(held.valid());
    EXPECT_EQ(held.hardware_id(), 0u);
    held.setup();
}

TEST_F(HeldInputTest, PassesThroughIdentityOfTheInnerInput)
{
    load(1000);
    EXPECT_EQ(held.hardware_id(), pin_id(4));
    EXPECT_EQ(held.get_inner_input(), button);
    EXPECT_TRUE(held.valid());
    button->is_valid = false;
    EXPECT_FALSE(held.valid());
    button->independent_analog = true;
    EXPECT_TRUE(held.has_independent_analog_value());
    held.setup();
    EXPECT_EQ(button->setups, 1);
    // Not a shortcut unless it wraps one
    EXPECT_EQ(held.as_shortcut(), nullptr);
}

TEST_F(HeldInputTest, HeldShortcutIsTreatedAsTheShortcut)
{
    // A held chord (eg hold Start + Select for the bootloader) still masks its member buttons
    auto chord = std::make_unique<ShortcutInput>();
    ShortcutInput *raw = chord.get();
    held.load(held_for(3000), std::move(chord));
    EXPECT_EQ(held.as_shortcut(), raw);
}

TEST_F(HeldInputTest, ZeroTimeStillWaitsAMillisecond)
{
    // The UI allows 0 ms (min 0). With a strict "longer than" check that is 1 ms, not instant.
    load(0);
    button->set(true);
    EXPECT_FALSE(held.tick_digital());
    clock_ms::advance(1);
    EXPECT_TRUE(held.tick_digital());
}

TEST_F(HeldInputTest, MillisWrapDoesNotCutTheHoldShort)
{
    // The press time is kept as a 32 bit millis() value, like ToggleDevice / CycleDevice, so the
    // elapsed time wraps correctly when a press straddles millis() wrapping (every ~49.7 days of
    // uptime) and eg a 3000 ms bootloader hold still needs the full 3000 ms.
    load(3000);
    clock_ms::set(0x100000000ull - 5); // 5 ms before millis() wraps
    button->set(true);
    EXPECT_FALSE(held.tick_digital());
    clock_ms::advance(10); // millis() is now 5, 10 ms after the press
    EXPECT_FALSE(held.tick_digital());
    clock_ms::advance(2991);
    EXPECT_TRUE(held.tick_digital());
}
