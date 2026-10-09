// The per profile logic in src/profiles/profile.cpp:
//  - release_to_triggers: an inactive profile keeps only what it needs to notice activating.
//  - Combined strum debounce (main.combinedStrumDebounce): "Strum up and down share one debounce
//    window, so a strum is ignored while the other direction is still held or debouncing".
//  - The input queue (main.queueInputs): "every fret / strum change is queued, then sent one at a
//    time so the game sees every change, even ones faster than its frame rate", each "held for"
//    the dequeue interval (main.dequeueInterval).
//  - Actions, run from the profile every loop (update_actions).
#include "inputs_test_support.hpp"
#include "input/shortcut.hpp"
#include "input/held.hpp"

namespace
{
proto_Mapping output(pb_size_t tag, int value)
{
    proto_Mapping config;
    memset(&config, 0, sizeof(config));
    config.mapping.which_mapping = tag;
    // Every output enum lives in the same union, so set it through the gamepad one
    config.mapping.mapping.gamepadButton = GamepadButtonType(value);
    return config;
}

proto_Output out(pb_size_t tag, int value)
{
    return output(tag, value).mapping;
}

class ProfileTest : public InputsTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile(GuitarHeroGuitar);

    ButtonMapping *add(proto_Mapping config, std::unique_ptr<Input> input)
    {
        auto mapping = std::make_unique<ButtonMapping>(config, std::move(input), (uint16_t)profile->mappings.size(), profile);
        ButtonMapping *raw = mapping.get();
        profile->mappings.push_back(std::move(mapping));
        // Same as load_mapping in src/config/config.cpp
        int8_t bit = InputQueue::bit_for_output(config.mapping, profile->subtype);
        if (profile->input_queue.enabled && bit >= 0)
        {
            raw->set_queue_bit(bit);
            profile->queued_mappings.push_back(raw);
        }
        if (bit == 10 || bit == 11)
        {
            raw->set_strum_bit(bit);
            profile->strum_mappings.push_back(raw);
        }
        return raw;
    }

    // One pass of the main loop as ProfileManager::update and an emulated device run it: the
    // queue and actions every loop, then a report built from every mapping
    void loop(uint64_t us = 1000)
    {
        profile->sample_input_queue();
        profile->update_actions();
        for (auto &mapping : profile->mappings)
        {
            mapping->update(false, false);
        }
        fake_time::advance_us(us);
    }
};

class StrumDebounceTest : public ProfileTest
{
protected:
    ButtonMapping *up = nullptr;
    ButtonMapping *down = nullptr;
    TestInput *up_switch = nullptr;
    TestInput *down_switch = nullptr;
    void SetUp() override
    {
        ProfileTest::SetUp();
        profile->combined_strum_debounce = true;
        up = add(with_debounce_ms(button_config(Gamepad_DpadUp), 10), test_input(up_switch));
        down = add(with_debounce_ms(button_config(Gamepad_DpadDown), 10), test_input(down_switch));
    }
    void sample()
    {
        up->update(false, false);
        down->update(false, false);
    }
};

class InputQueueTest : public ProfileTest
{
protected:
    ButtonMapping *green = nullptr;
    TestInput *green_fret = nullptr;
    void SetUp() override
    {
        ProfileTest::SetUp();
        profile->input_queue.enabled = true;
        profile->input_queue.interval_us = 1000;
        green = add(output(proto_Output_ghButton_tag, GuitarHeroGuitar_Green), test_input(green_fret));
    }
    // What a report built now sends for a queue bit
    bool shown(int8_t bit) { return profile->input_queue.presented(bit); }
};
} // namespace

TEST_F(ProfileTest, ReleaseToTriggersKeepsOnlyTheTriggers)
{
    int offs = 0;
    profile->leds.push_back(std::make_unique<TestLedMapping>(std::make_unique<CountingLedDevice>(&offs), profile, 0));
    profile->leds.push_back(std::make_unique<TestLedMapping>(std::make_unique<CountingLedDevice>(&offs), profile, 1));
    profile->input_queue.enabled = true;
    add(output(proto_Output_ghButton_tag, GuitarHeroGuitar_Green), std::make_unique<TestInput>());
    add(button_config(Gamepad_DpadUp), std::make_unique<TestInput>());
    profile->triggers.push_back(std::make_unique<ActivationTriggerList>());
    ASSERT_FALSE(profile->queued_mappings.empty());
    ASSERT_FALSE(profile->strum_mappings.empty());

    profile->release_to_triggers();
    // The LEDs are turned off before they're dropped, so they don't stay lit
    EXPECT_EQ(offs, 2);
    EXPECT_TRUE(profile->leds.empty());
    EXPECT_TRUE(profile->mappings.empty());
    EXPECT_TRUE(profile->queued_mappings.empty());
    EXPECT_TRUE(profile->queued_last_live.empty());
    EXPECT_TRUE(profile->strum_mappings.empty());
    EXPECT_TRUE(profile->action_mappings.empty());
    EXPECT_EQ(profile->triggers.size(), 1u);
    EXPECT_TRUE(profile->triggers_only);
    // Nothing left to sample
    profile->sample_input_queue();
    profile->update_actions();
}

TEST_F(StrumDebounceTest, OtherDirectionIsIgnoredWhileOneIsHeld)
{
    up_switch->set(true);
    sample();
    EXPECT_TRUE(up->live_value());
    clock_ms::advance(30);
    down_switch->set(true);
    sample();
    EXPECT_TRUE(up->live_value());
    EXPECT_FALSE(down->live_value());
}

TEST_F(StrumDebounceTest, OtherDirectionIsIgnoredWhileOneIsDebouncing)
{
    up_switch->set(true);
    sample();
    up_switch->set(false);
    clock_ms::advance(5);
    sample();
    // Up is still live for its 10 ms debounce
    ASSERT_TRUE(up->live_value());
    down_switch->set(true);
    sample();
    EXPECT_FALSE(down->live_value());
    // A bounce on down that's over before up finishes debouncing never registers
    down_switch->set(false);
    clock_ms::advance(4);
    sample();
    EXPECT_FALSE(down->live_value());
    clock_ms::advance(2);
    sample();
    EXPECT_FALSE(up->live_value());
    EXPECT_FALSE(down->live_value());
    // A real down strum after that works
    down_switch->set(true);
    sample();
    EXPECT_TRUE(down->live_value());
}

TEST_F(StrumDebounceTest, WorksBothWays)
{
    down_switch->set(true);
    sample();
    up_switch->set(true);
    sample();
    EXPECT_TRUE(down->live_value());
    EXPECT_FALSE(up->live_value());
}

TEST_F(StrumDebounceTest, StrumHeldThroughTheWindowRegistersAfterIt)
{
    // Ignored while the other direction is live, but a strum still held once it isn't goes through
    up_switch->set(true);
    sample();
    down_switch->set(true);
    sample();
    up_switch->set(false);
    clock_ms::advance(10);
    sample();
    EXPECT_FALSE(down->live_value());
    clock_ms::advance(1);
    sample();
    EXPECT_FALSE(up->live_value());
    EXPECT_TRUE(down->live_value());
}

TEST_F(StrumDebounceTest, BothAtOnceGoesToTheFirstMapping)
{
    up_switch->set(true);
    down_switch->set(true);
    sample();
    EXPECT_TRUE(up->live_value());
    EXPECT_FALSE(down->live_value());
}

TEST_F(StrumDebounceTest, OffMeansIndependentStrums)
{
    profile->combined_strum_debounce = false;
    up_switch->set(true);
    sample();
    down_switch->set(true);
    sample();
    EXPECT_TRUE(up->live_value());
    EXPECT_TRUE(down->live_value());
}

TEST_F(StrumDebounceTest, OtherButtonsAreUnaffected)
{
    TestInput *fret = nullptr;
    auto *green = add(output(proto_Output_ghButton_tag, GuitarHeroGuitar_Green), test_input(fret));
    up_switch->set(true);
    sample();
    fret->set(true);
    green->update(false, false);
    EXPECT_TRUE(green->live_value());
    EXPECT_FALSE(profile->other_strum_live(10));
    EXPECT_TRUE(profile->other_strum_live(11));
}

TEST_F(ProfileTest, QueueBitsForEachOutput)
{
    using IQ = InputQueue;
    for (int fret = GuitarHeroGuitar_Green; fret <= GuitarHeroGuitar_Orange; fret++)
    {
        EXPECT_EQ(IQ::bit_for_output(out(proto_Output_ghButton_tag, fret), GuitarHeroGuitar), fret - 1);
    }
    for (int fret = RockBandGuitar_Green; fret <= RockBandGuitar_SoloOrange; fret++)
    {
        EXPECT_EQ(IQ::bit_for_output(out(proto_Output_rbButton_tag, fret), RockBandGuitar), fret - 1);
    }
    for (int fret = GuitarHeroLiveGuitar_White1; fret <= GuitarHeroLiveGuitar_Black3; fret++)
    {
        EXPECT_EQ(IQ::bit_for_output(out(proto_Output_ghlButton_tag, fret), LiveGuitar), fret - 1);
    }
    EXPECT_EQ(IQ::bit_for_output(out(proto_Output_ghlButton_tag, GuitarHeroLiveGuitar_StrumUp), LiveGuitar), 10);
    EXPECT_EQ(IQ::bit_for_output(out(proto_Output_ghlButton_tag, GuitarHeroLiveGuitar_StrumDown), LiveGuitar), 11);
    // Strums on guitars are the d-pad
    for (SubType guitar : {GuitarHeroGuitar, RockBandGuitar, LiveGuitar})
    {
        EXPECT_EQ(IQ::bit_for_output(out(proto_Output_gamepadButton_tag, Gamepad_DpadUp), guitar), 10);
        EXPECT_EQ(IQ::bit_for_output(out(proto_Output_gamepadButton_tag, Gamepad_DpadDown), guitar), 11);
        EXPECT_EQ(IQ::bit_for_output(out(proto_Output_gamepadButton_tag, Gamepad_Start), guitar), -1);
    }
    // GuitarFreaks: only its single strum queues
    EXPECT_EQ(IQ::bit_for_output(out(proto_Output_gfButton_tag, GuitarFreaks_Strum), GuitarFreaks), 10);
    EXPECT_EQ(IQ::bit_for_output(out(proto_Output_gfButton_tag, GuitarFreaks_Red), GuitarFreaks), -1);
    EXPECT_EQ(IQ::bit_for_output(out(proto_Output_gamepadButton_tag, Gamepad_DpadUp), GuitarFreaks), -1);
    // Wrong instrument's outputs and non guitars don't queue
    EXPECT_EQ(IQ::bit_for_output(out(proto_Output_rbButton_tag, RockBandGuitar_Green), GuitarHeroGuitar), -1);
    EXPECT_EQ(IQ::bit_for_output(out(proto_Output_gamepadButton_tag, Gamepad_DpadUp), Gamepad), -1);
    EXPECT_EQ(IQ::bit_for_output(out(proto_Output_ghButton_tag, GuitarHeroGuitar_Green), RockBandDrums), -1);
}

TEST_F(InputQueueTest, OffDoesNothing)
{
    profile->input_queue.enabled = false;
    green_fret->set(true);
    profile->sample_input_queue();
    EXPECT_FALSE(shown(0));
    EXPECT_EQ(profile->input_queue.last_pushed(), 0);
}

TEST_F(InputQueueTest, TapShorterThanAFrameIsStillShownForTheInterval)
{
    green_fret->set(true);
    loop(200);
    EXPECT_TRUE(shown(0));
    green_fret->set(false);
    // Released after 0.2 ms, but shown until a full interval has passed
    for (int i = 0; i < 4; i++)
    {
        loop(200);
        EXPECT_TRUE(shown(0)) << i;
    }
    loop(200);
    EXPECT_FALSE(shown(0));
}

TEST_F(InputQueueTest, EveryChangeIsShownInOrder)
{
    TestInput *red_fret = nullptr;
    add(output(proto_Output_ghButton_tag, GuitarHeroGuitar_Red), test_input(red_fret));
    // Prime the queue so a report is consuming it
    loop(2000);
    // green, green + red, red, nothing - all within 0.3 ms
    green_fret->set(true);
    loop(100);
    red_fret->set(true);
    loop(100);
    green_fret->set(false);
    loop(100);
    red_fret->set(false);
    std::vector<std::pair<bool, bool>> seen;
    for (int i = 0; i < 60; i++)
    {
        loop(100);
        std::pair<bool, bool> now{shown(0), shown(1)};
        if (seen.empty() || seen.back() != now)
        {
            seen.push_back(now);
        }
    }
    std::vector<std::pair<bool, bool>> expected = {{true, false}, {true, true}, {false, true}, {false, false}};
    EXPECT_EQ(seen, expected);
}

TEST_F(InputQueueTest, NobodyReadingCollapsesToTheLatestState)
{
    // With no reports going out (nothing reads the presented state) the backlog isn't replayed
    loop(2000);
    for (int i = 0; i < 10; i++)
    {
        green_fret->set(i % 2 == 0);
        profile->sample_input_queue();
        fake_time::advance_us(20000);
    }
    green_fret->set(true);
    profile->sample_input_queue();
    fake_time::advance_us(InputQueue::STALE_US + 1);
    profile->sample_input_queue();
    EXPECT_TRUE(shown(0));
    loop(1000);
    loop(1000);
    EXPECT_TRUE(shown(0));
}

TEST_F(InputQueueTest, SharedOutputIsReleasedBetweenPresses)
{
    // GuitarFreaks has one strum output fed by both strum switches: picking the other way while
    // the first is still held needs a release in between, or the game would see one long strum
    profile = make_profile(GuitarFreaks);
    profile->input_queue.enabled = true;
    profile->input_queue.interval_us = 1000;
    TestInput *up_switch = nullptr;
    TestInput *down_switch = nullptr;
    add(output(proto_Output_gfButton_tag, GuitarFreaks_Strum), test_input(up_switch));
    add(output(proto_Output_gfButton_tag, GuitarFreaks_Strum), test_input(down_switch));
    loop(2000);
    up_switch->set(true);
    loop(100);
    down_switch->set(true);
    std::vector<bool> seen;
    for (int i = 0; i < 60; i++)
    {
        loop(100);
        if (seen.empty() || seen.back() != shown(10))
        {
            seen.push_back(shown(10));
        }
    }
    EXPECT_EQ(seen, (std::vector<bool>{true, false, true}));
}

TEST_F(InputQueueTest, QueuedButtonsStillDebounce)
{
    // The queue takes the debounced state, so a fret's debounce still holds it on
    TestInput *red_fret = nullptr;
    add(with_debounce_ms(output(proto_Output_ghButton_tag, GuitarHeroGuitar_Red), 5), test_input(red_fret));
    loop(2000);
    red_fret->set(true);
    loop(1000);
    red_fret->set(false);
    for (int i = 0; i < 4; i++)
    {
        loop(1000);
        EXPECT_TRUE(shown(1)) << i;
    }
    for (int i = 0; i < 5; i++)
    {
        loop(1000);
    }
    EXPECT_FALSE(shown(1));
}

TEST_F(InputQueueTest, ShortcutMaskingStillAppliesToQueuedFrets)
{
    // With the input queue on (queueInputs, or always for GuitarFreaks) ProfileManager::update
    // samples the queued mappings before any report is built. Profile::sample_input_queue lets
    // each shortcut mask its queued fret / strum members first, so the fret's press never goes
    // into the queue and isn't shown to the game alongside the shortcut's output, the same as
    // without the queue (where resolve_shortcuts evaluates the shortcut first).
    constexpr uint8_t START = 9;
    constexpr uint8_t GREEN = 2;
    profile->mappings.clear();
    profile->queued_mappings.clear();
    add(output(proto_Output_ghButton_tag, GuitarHeroGuitar_Green), pin(GREEN));
    add(button_config(Gamepad_Start), pin(START));
    auto chord = std::make_unique<ShortcutInput>();
    chord->inputs.push_back(pin(START));
    chord->inputs.push_back(pin(GREEN));
    auto *guide = add(button_config(Gamepad_Guide), std::move(chord));
    profile->resolve_shortcuts();

    loop();
    fake_pins::state[START] = true;
    loop();
    fake_pins::state[GREEN] = true;
    for (int i = 0; i < 10; i++)
    {
        loop();
        EXPECT_TRUE(guide->live_value());
        EXPECT_FALSE(shown(0)) << "green sent with the shortcut, loop " << i;
    }
}

TEST_F(ProfileTest, BootloaderActionNeedsTheFullHold)
{
    // The configurator wraps actions in a held input: 3000 ms for the bootloader
    proto_Mapping config = output(proto_Output_action_tag, ActionBootloader);
    proto_HeldInput held_config = proto_HeldInput_init_default;
    held_config.time = 3000;
    auto held = std::make_unique<HeldInput>();
    TestInput *button = nullptr;
    held->load(held_config, test_input(button));
    auto mapping = std::make_unique<ActionMapping>(config, std::move(held), 0, profile);
    profile->action_mappings.push_back(mapping.get());
    profile->mappings.push_back(std::move(mapping));

    profile->update_actions();
    button->set(true);
    for (int i = 0; i <= 3000; i++)
    {
        profile->update_actions();
        clock_ms::advance(1);
    }
    EXPECT_EQ(fake_bootrom::reset_count, 0);
    profile->update_actions();
    EXPECT_EQ(fake_bootrom::reset_count, 1);
    // Keeping it held doesn't run it again
    clock_ms::advance(5000);
    profile->update_actions();
    EXPECT_EQ(fake_bootrom::reset_count, 1);
}

TEST_F(ProfileTest, ActionHeldAtStartNeedsReleasingFirst)
{
    // Restarting the device stack while still holding the button mustn't restart it again
    proto_Mapping config = output(proto_Output_action_tag, ActionRestartDeviceStack);
    TestInput *button = nullptr;
    auto mapping = std::make_unique<ActionMapping>(config, test_input(button), 0, profile);
    profile->action_mappings.push_back(mapping.get());
    profile->mappings.push_back(std::move(mapping));
    button->set(true);
    profile->update_actions();
    EXPECT_FALSE(ConfigManager::instance().take_device_stack_restart());
    button->set(false);
    profile->update_actions();
    button->set(true);
    profile->update_actions();
    EXPECT_TRUE(ConfigManager::instance().take_device_stack_restart());
    profile->update_actions();
    EXPECT_FALSE(ConfigManager::instance().take_device_stack_restart());
}
