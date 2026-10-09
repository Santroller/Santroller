// Shortcuts: a chord of inputs that only counts while every one of them is pressed (eg the
// guitar guide's "map Start + Select to the Guide / Home button"). Profile::resolve_shortcuts
// makes the chord mask the plain mappings of its member buttons, so pressing the chord sends
// only the chord's output, and ButtonMapping::sample keeps a masked button off until it has
// been physically released (the release latch), so letting go of the chord one button at a
// time doesn't leak the button still held.
#include "inputs_test_support.hpp"
#include "input/shortcut.hpp"
#include "input/held.hpp"
#include "input/shifted.hpp"

namespace
{
std::unique_ptr<ShortcutInput> chord(std::initializer_list<uint8_t> pins)
{
    auto shortcut = std::make_unique<ShortcutInput>();
    for (auto number : pins)
    {
        shortcut->inputs.push_back(pin(number));
    }
    return shortcut;
}

constexpr uint8_t START = 9;
constexpr uint8_t SELECT = 10;
constexpr uint8_t A = 2;

class ShortcutMaskTest : public InputsTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile();

    ButtonMapping *add(GamepadButtonType button, std::unique_ptr<Input> input)
    {
        auto mapping = std::make_unique<ButtonMapping>(button_config(button), std::move(input),
                                                       (uint16_t)profile->mappings.size(), profile);
        ButtonMapping *raw = mapping.get();
        profile->mappings.push_back(std::move(mapping));
        return raw;
    }

    // One report: every mapping in the profile's order, as the emulated devices do
    void loop()
    {
        for (auto &mapping : profile->mappings)
        {
            mapping->update(false, false);
        }
        clock_ms::advance(1);
    }

    void press(uint8_t number, bool pressed = true) { fake_pins::state[number] = pressed; }
};
} // namespace

TEST(ShortcutInput, OnlyPressedWhileEveryMemberIsPressed)
{
    fake_pins::reset();
    auto shortcut = chord({START, SELECT});
    EXPECT_FALSE(shortcut->tick_digital());
    fake_pins::state[START] = true;
    EXPECT_FALSE(shortcut->tick_digital());
    EXPECT_EQ(shortcut->tick_analog(), 0);
    fake_pins::state[SELECT] = true;
    EXPECT_TRUE(shortcut->tick_digital());
    EXPECT_EQ(shortcut->tick_analog(), UINT16_MAX);
    fake_pins::state[START] = false;
    EXPECT_FALSE(shortcut->tick_digital());
    fake_pins::reset();
}

TEST(ShortcutInput, EmptyShortcutIsNeverPressedAndInvalid)
{
    ShortcutInput shortcut;
    EXPECT_FALSE(shortcut.tick_digital());
    EXPECT_EQ(shortcut.tick_analog(), 0);
    EXPECT_FALSE(shortcut.valid());
}

TEST(ShortcutInput, ValidOnlyWhenEveryMemberIs)
{
    ShortcutInput shortcut;
    TestInput *first = nullptr;
    TestInput *second = nullptr;
    shortcut.inputs.push_back(test_input(first));
    shortcut.inputs.push_back(test_input(second));
    EXPECT_TRUE(shortcut.valid());
    second->is_valid = false;
    EXPECT_FALSE(shortcut.valid());
    shortcut.setup();
    EXPECT_EQ(first->setups, 1);
    EXPECT_EQ(second->setups, 1);
    EXPECT_EQ(shortcut.as_shortcut(), &shortcut);
}

TEST(ShortcutInput, SingleMemberShortcutFollowsThatMember)
{
    fake_pins::reset();
    auto shortcut = chord({START});
    EXPECT_FALSE(shortcut->tick_digital());
    fake_pins::state[START] = true;
    EXPECT_TRUE(shortcut->tick_digital());
    fake_pins::reset();
}

TEST_F(ShortcutMaskTest, ShortcutsMoveAheadOfTheMappingsTheyMask)
{
    auto *start = add(Gamepad_Start, pin(START));
    auto *guide = add(Gamepad_Guide, chord({START, SELECT}));
    auto *select = add(Gamepad_Back, pin(SELECT));
    auto *capture = add(Gamepad_Capture, chord({A, SELECT}));
    profile->resolve_shortcuts();
    ASSERT_EQ(profile->mappings.size(), 4u);
    // Shortcuts first in their original order, then the rest in theirs
    EXPECT_EQ(profile->mappings[0].get(), guide);
    EXPECT_EQ(profile->mappings[1].get(), capture);
    EXPECT_EQ(profile->mappings[2].get(), start);
    EXPECT_EQ(profile->mappings[3].get(), select);
}

TEST_F(ShortcutMaskTest, ChordSendsOnlyTheShortcutOutput)
{
    auto *start = add(Gamepad_Start, pin(START));
    auto *select = add(Gamepad_Back, pin(SELECT));
    auto *guide = add(Gamepad_Guide, chord({START, SELECT}));
    profile->resolve_shortcuts();

    press(START);
    press(SELECT);
    loop();
    EXPECT_TRUE(guide->live_value());
    EXPECT_FALSE(start->live_value());
    EXPECT_FALSE(select->live_value());
}

TEST_F(ShortcutMaskTest, MemberPressedFirstIsCutOffOnceTheChordCompletes)
{
    auto *start = add(Gamepad_Start, pin(START));
    auto *select = add(Gamepad_Back, pin(SELECT));
    auto *guide = add(Gamepad_Guide, chord({START, SELECT}));
    profile->resolve_shortcuts();

    press(START);
    loop();
    // On its own, Start is just Start
    EXPECT_TRUE(start->live_value());
    EXPECT_FALSE(guide->live_value());

    press(SELECT);
    loop();
    EXPECT_TRUE(guide->live_value());
    EXPECT_FALSE(start->live_value());
    EXPECT_FALSE(select->live_value());
}

TEST_F(ShortcutMaskTest, ButtonStillHeldAfterTheChordStaysOffUntilReleased)
{
    auto *start = add(Gamepad_Start, pin(START));
    auto *select = add(Gamepad_Back, pin(SELECT));
    auto *guide = add(Gamepad_Guide, chord({START, SELECT}));
    profile->resolve_shortcuts();

    press(START);
    press(SELECT);
    loop();
    ASSERT_TRUE(guide->live_value());

    // Let go of Select first: the chord is over, but Start mustn't suddenly press
    press(SELECT, false);
    for (int i = 0; i < 50; i++)
    {
        loop();
        EXPECT_FALSE(guide->live_value());
        EXPECT_FALSE(start->live_value());
        EXPECT_FALSE(select->live_value());
    }

    // Once Start has been released it works on its own again
    press(START, false);
    loop();
    EXPECT_FALSE(start->live_value());
    press(START);
    loop();
    EXPECT_TRUE(start->live_value());
    EXPECT_FALSE(guide->live_value());
}

TEST_F(ShortcutMaskTest, LatchWorksWhicheverMemberIsReleasedFirst)
{
    auto *start = add(Gamepad_Start, pin(START));
    auto *select = add(Gamepad_Back, pin(SELECT));
    add(Gamepad_Guide, chord({START, SELECT}));
    profile->resolve_shortcuts();

    press(START);
    press(SELECT);
    loop();
    press(START, false);
    loop();
    EXPECT_FALSE(select->live_value());
    EXPECT_FALSE(start->live_value());
    press(SELECT, false);
    loop();
    press(SELECT);
    loop();
    EXPECT_TRUE(select->live_value());
}

TEST_F(ShortcutMaskTest, LatchIgnoresDebounceHold)
{
    // A debounced member would normally stay live for the debounce time after its press; a
    // shortcut masks it straight away instead
    auto *start = add(Gamepad_Start, pin(START));
    profile->mappings.back() = std::make_unique<ButtonMapping>(with_debounce_ms(button_config(Gamepad_Start), 50), pin(START), 0, profile);
    start = profile->mappings.back()->as_button_mapping();
    add(Gamepad_Guide, chord({START, SELECT}));
    profile->resolve_shortcuts();

    press(START);
    loop();
    ASSERT_TRUE(start->live_value());
    press(SELECT);
    loop();
    EXPECT_FALSE(start->live_value());
}

TEST_F(ShortcutMaskTest, OtherButtonsAreUnaffected)
{
    auto *a = add(Gamepad_A, pin(A));
    add(Gamepad_Guide, chord({START, SELECT}));
    profile->resolve_shortcuts();
    press(START);
    press(SELECT);
    press(A);
    loop();
    EXPECT_TRUE(a->live_value());
}

TEST_F(ShortcutMaskTest, WrappedMemberInputsAreMaskedToo)
{
    // Masking matches the physical input, so a held or shifted mapping on a member button is the
    // same button and is masked as well
    proto_HeldInput held_config = proto_HeldInput_init_default;
    held_config.time = 0;
    auto held = std::make_unique<HeldInput>();
    held->load(held_config, pin(START));
    auto *held_start = add(Gamepad_X, std::move(held));

    proto_ShiftedInput shifted_config = proto_ShiftedInput_init_default;
    auto shifted = std::make_unique<ShiftedInput>();
    shifted->load(shifted_config, pin(SELECT), pin(A));
    auto *shifted_select = add(Gamepad_Y, std::move(shifted));

    add(Gamepad_Guide, chord({START, SELECT}));
    profile->resolve_shortcuts();

    press(A);
    press(START);
    press(SELECT);
    for (int i = 0; i < 5; i++)
    {
        loop();
        EXPECT_FALSE(held_start->live_value());
        EXPECT_FALSE(shifted_select->live_value());
    }
}

TEST_F(ShortcutMaskTest, HeldChordMasksItsMembersWhileBeingHeld)
{
    // Hold Start + Select for 3 s to do something: neither Start nor Select should be sent during
    // the hold, and the chord's own output waits for the hold time
    auto *start = add(Gamepad_Start, pin(START));
    auto *select = add(Gamepad_Back, pin(SELECT));
    proto_HeldInput held_config = proto_HeldInput_init_default;
    held_config.time = 3000;
    auto held = std::make_unique<HeldInput>();
    held->load(held_config, chord({START, SELECT}));
    auto *guide = add(Gamepad_Guide, std::move(held));
    profile->resolve_shortcuts();
    ASSERT_EQ(profile->mappings[0].get(), guide);

    press(START);
    press(SELECT);
    // Each loop is 1 ms; the hold starts on the first one and needs longer than 3000 ms
    for (int i = 0; i <= 3000; i++)
    {
        loop();
        ASSERT_FALSE(guide->live_value()) << "after " << i << " ms";
        ASSERT_FALSE(start->live_value());
        ASSERT_FALSE(select->live_value());
    }
    loop();
    EXPECT_TRUE(guide->live_value());
    EXPECT_FALSE(start->live_value());
}

TEST_F(ShortcutMaskTest, ShortcutsDoNotMaskEachOther)
{
    // Two chords sharing members both fire; only plain mappings get masked
    auto *guide = add(Gamepad_Guide, chord({START, SELECT}));
    auto *capture = add(Gamepad_Capture, chord({START, SELECT, A}));
    profile->resolve_shortcuts();
    press(START);
    press(SELECT);
    press(A);
    loop();
    EXPECT_TRUE(guide->live_value());
    EXPECT_TRUE(capture->live_value());
}

TEST_F(ShortcutMaskTest, MembersWithoutAHardwareIdMaskNothing)
{
    // Inputs that can't say which physical input they are (hardware id 0) are left alone
    TestInput *member = nullptr;
    TestInput *plain = nullptr;
    auto shortcut = std::make_unique<ShortcutInput>();
    shortcut->inputs.push_back(test_input(member));
    auto *guide = add(Gamepad_Guide, std::move(shortcut));
    auto *a = add(Gamepad_A, test_input(plain));
    profile->resolve_shortcuts();
    member->set(true);
    plain->set(true);
    loop();
    EXPECT_TRUE(guide->live_value());
    EXPECT_TRUE(a->live_value());
}

TEST_F(ShortcutMaskTest, ResolvingAgainDoesNotChangeBehaviour)
{
    auto *start = add(Gamepad_Start, pin(START));
    auto *guide = add(Gamepad_Guide, chord({START, SELECT}));
    profile->resolve_shortcuts();
    profile->resolve_shortcuts();
    press(START);
    press(SELECT);
    loop();
    EXPECT_TRUE(guide->live_value());
    EXPECT_FALSE(start->live_value());
    press(SELECT, false);
    press(START, false);
    loop();
    press(START);
    loop();
    EXPECT_TRUE(start->live_value());
}

TEST_F(ShortcutMaskTest, WithoutResolvingNothingIsMasked)
{
    // The masks are only set up by resolve_shortcuts (ProfileManager::add_profile calls it)
    auto *start = add(Gamepad_Start, pin(START));
    add(Gamepad_Guide, chord({START, SELECT}));
    press(START);
    press(SELECT);
    loop();
    EXPECT_TRUE(start->live_value());
}

TEST_F(ShortcutMaskTest, SuppressedStateIsVisible)
{
    auto *start = add(Gamepad_Start, pin(START));
    add(Gamepad_Guide, chord({START, SELECT}));
    profile->resolve_shortcuts();
    EXPECT_FALSE(start->is_suppressed());
    press(START);
    press(SELECT);
    loop();
    EXPECT_TRUE(start->is_suppressed());
    press(START, false);
    press(SELECT, false);
    loop();
    EXPECT_FALSE(start->is_suppressed());
}
