// Shifted inputs: a "Primary Input" that only counts while the "Shift / Modifier Input" is held,
// or, with "Invert Shift (active when modifier is released)", only while it isn't. The
// configurator's Rock Band dual contact neck driver uses a pair per fret on one solo line:
// the main fret is shifted with invertShift, the solo fret without it, so exactly one of them is
// pressed whenever the fret is.
#include "inputs_test_support.hpp"
#include "input/shifted.hpp"

namespace
{
std::unique_ptr<ShiftedInput> shifted(std::unique_ptr<Input> input, std::unique_ptr<Input> shift, bool invert,
                                      bool has_invert = true)
{
    proto_ShiftedInput config = proto_ShiftedInput_init_default;
    config.has_invertShift = has_invert;
    config.invertShift = invert;
    auto result = std::make_unique<ShiftedInput>();
    result->load(config, std::move(input), std::move(shift));
    return result;
}

constexpr uint8_t FRET = 2;
constexpr uint8_t SOLO = 12;

class ShiftedTest : public InputsTest
{
};
} // namespace

TEST_F(ShiftedTest, OnlyPressedWhileShiftIsHeld)
{
    auto input = shifted(pin(FRET), pin(SOLO), false);
    EXPECT_FALSE(input->tick_digital());
    fake_pins::state[FRET] = true;
    EXPECT_FALSE(input->tick_digital());
    fake_pins::state[SOLO] = true;
    EXPECT_TRUE(input->tick_digital());
    fake_pins::state[FRET] = false;
    EXPECT_FALSE(input->tick_digital());
}

TEST_F(ShiftedTest, InvertedOnlyPressedWhileShiftIsReleased)
{
    auto input = shifted(pin(FRET), pin(SOLO), true);
    fake_pins::state[FRET] = true;
    EXPECT_TRUE(input->tick_digital());
    fake_pins::state[SOLO] = true;
    EXPECT_FALSE(input->tick_digital());
    fake_pins::state[FRET] = false;
    fake_pins::state[SOLO] = false;
    EXPECT_FALSE(input->tick_digital());
}

TEST_F(ShiftedTest, MissingInvertMeansNotInverted)
{
    auto input = shifted(pin(FRET), pin(SOLO), true, false);
    fake_pins::state[FRET] = true;
    EXPECT_FALSE(input->tick_digital());
    fake_pins::state[SOLO] = true;
    EXPECT_TRUE(input->tick_digital());
}

TEST_F(ShiftedTest, RockBandSoloFretsSplitOneFretIntoTwoLayers)
{
    // Main and solo green on the same pins, as configureRbShiftedFrets sets them up
    auto green = shifted(pin(FRET), pin(SOLO), true);
    auto solo_green = shifted(pin(FRET), pin(SOLO), false);
    struct Step
    {
        bool fret, solo, green, solo_green;
    };
    const Step steps[] = {
        {false, false, false, false},
        {true, false, true, false},
        // Sliding up to the solo frets: the solo line closes with the fret
        {true, true, false, true},
        {true, false, true, false},
        // A solo line on its own (another solo fret's contact) presses nothing here
        {false, true, false, false},
        {false, false, false, false},
    };
    for (const auto &step : steps)
    {
        fake_pins::state[FRET] = step.fret;
        fake_pins::state[SOLO] = step.solo;
        EXPECT_EQ(green->tick_digital(), step.green) << step.fret << step.solo;
        EXPECT_EQ(solo_green->tick_digital(), step.solo_green) << step.fret << step.solo;
    }
}

TEST_F(ShiftedTest, AnalogPassesThroughOnlyInTheActiveLayer)
{
    TestInput *whammy = nullptr;
    auto input = shifted(test_input(whammy), pin(SOLO), false);
    whammy->analog = 40000;
    EXPECT_EQ(input->tick_analog(), 0);
    fake_pins::state[SOLO] = true;
    EXPECT_EQ(input->tick_analog(), 40000);

    TestInput *other = nullptr;
    auto inverted = shifted(test_input(other), pin(SOLO), true);
    other->analog = 1234;
    EXPECT_EQ(inverted->tick_analog(), 0);
    fake_pins::state[SOLO] = false;
    EXPECT_EQ(inverted->tick_analog(), 1234);
}

TEST_F(ShiftedTest, MissingEitherInputIsNeverPressed)
{
    fake_pins::state[FRET] = true;
    fake_pins::state[SOLO] = true;
    auto no_shift = shifted(pin(FRET), nullptr, true);
    EXPECT_FALSE(no_shift->tick_digital());
    EXPECT_EQ(no_shift->tick_analog(), 0);
    EXPECT_FALSE(no_shift->valid());
    auto no_input = shifted(nullptr, pin(SOLO), false);
    EXPECT_FALSE(no_input->tick_digital());
    EXPECT_EQ(no_input->tick_analog(), 0);
    EXPECT_FALSE(no_input->valid());
    no_input->setup();
    no_shift->setup();
}

TEST_F(ShiftedTest, IdentityIsThePrimaryInput)
{
    TestInput *primary = nullptr;
    TestInput *shift = nullptr;
    auto input = shifted(test_input(primary, pin_id(FRET)), test_input(shift, pin_id(SOLO)), false);
    EXPECT_EQ(input->hardware_id(), pin_id(FRET));
    EXPECT_EQ(input->get_inner_input(), primary);
    EXPECT_EQ(input->get_shift_input(), shift);
    EXPECT_EQ(input->as_shortcut(), nullptr);
    EXPECT_TRUE(input->valid());
    shift->is_valid = false;
    EXPECT_FALSE(input->valid());
    primary->independent_analog = true;
    EXPECT_TRUE(input->has_independent_analog_value());
    input->setup();
    EXPECT_EQ(primary->setups, 1);
    EXPECT_EQ(shift->setups, 1);
}

TEST_F(ShiftedTest, LayerSwitchMidPressMovesTheOutput)
{
    // Pressing the modifier while the primary is held hands the press from one layer to the other
    // straight away (no latch)
    auto profile = make_profile();
    auto base = std::make_unique<ButtonMapping>(button_config(Gamepad_A), shifted(pin(FRET), pin(SOLO), true), 0, profile);
    auto layer = std::make_unique<ButtonMapping>(button_config(Gamepad_X), shifted(pin(FRET), pin(SOLO), false), 1, profile);
    fake_pins::state[FRET] = true;
    base->update(false, false);
    layer->update(false, false);
    EXPECT_TRUE(base->live_value());
    EXPECT_FALSE(layer->live_value());
    fake_pins::state[SOLO] = true;
    base->update(false, false);
    layer->update(false, false);
    EXPECT_FALSE(base->live_value());
    EXPECT_TRUE(layer->live_value());
}
