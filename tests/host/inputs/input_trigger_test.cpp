// Input based profile activation ("Activation Condition" in the profile setup):
//  - "Hold Button at Startup" / assignmentTypeDesc.input: "Match if an input is held when
//    powering on this device". The firmware gives it the 2 s after the last mode change / reload.
//  - "Press Button Anytime" / assignmentTypeDesc.inputAnyTime: "Match on input is pressed at any
//    time". A press reloads the config so the profiles get reassigned.
// Plus ActivationTriggerList, which ANDs the triggers of one list ("logic_and") and latches once
// it has claimed its devices.
#include "inputs_test_support.hpp"
#include "triggers/input_trigger.hpp"

namespace
{
proto_InputActivationTrigger plain_trigger()
{
    proto_InputActivationTrigger config;
    memset(&config, 0, sizeof(config));
    return config;
}

proto_InputActivationTrigger analog_trigger(AnalogToDigitalTriggerType type, int32_t value, int32_t max = 0)
{
    proto_InputActivationTrigger config = plain_trigger();
    config.has_trigger = true;
    config.trigger = type;
    config.has_triggerValue = true;
    config.triggerValue = value;
    config.has_maxTriggerValue = max != 0;
    config.maxTriggerValue = max;
    return config;
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

// A trigger with a fixed answer, counting what the list does with it
class StubTrigger : public ActivationTrigger
{
public:
    StubTrigger(std::shared_ptr<Profile> profile, bool *matches, int assigned = 0)
        : ActivationTrigger(profile, 0, 0), m_matches(matches), m_assigned(assigned) {}
    bool validate(bool claim_device, bool, bool) override
    {
        validations++;
        if (claim_device)
        {
            claims++;
        }
        return *m_matches;
    }
    void reset() override { resets++; }
    int assignedDevices() override { return m_assigned; }
    bool forcedConsoleMode(ConsoleMode &mode) const override
    {
        if (!forced)
        {
            return false;
        }
        mode = *forced;
        return true;
    }
    int validations = 0;
    int claims = 0;
    int resets = 0;
    const ConsoleMode *forced = nullptr;

private:
    bool *m_matches;
    int m_assigned;
};

class InputTriggerTest : public InputsTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile();
    TestInput *button = nullptr;

    std::unique_ptr<InputActivationTrigger> make(bool any_time, proto_InputActivationTrigger config = plain_trigger())
    {
        return std::make_unique<InputActivationTrigger>(any_time, config, test_input(button), profile, 7, 2);
    }

    // The controller has just (re)loaded its config
    void boot() { ConfigManager::instance().finish_reinit(clock_ms::now()); }
};
} // namespace

TEST_F(InputTriggerTest, HeldAtStartupMatchesWithoutReloading)
{
    boot();
    auto input = std::make_unique<TestInput>();
    input->set(true);
    InputActivationTrigger trigger(false, plain_trigger(), std::move(input), profile, 7, 2);
    EXPECT_TRUE(trigger.validate(false, false, false));
    clock_ms::advance(1000);
    EXPECT_TRUE(trigger.validate(false, false, false));
    EXPECT_EQ(fake_config::reload_count, 0);
}

TEST_F(InputTriggerTest, StartupTriggerOnlyCountsJustAfterStartup)
{
    boot();
    auto trigger = make(false);
    button->set(true);
    clock_ms::advance(1999);
    EXPECT_TRUE(trigger->validate(false, false, false));
    clock_ms::advance(1);
    // 2 s after the reload the button no longer selects the profile
    EXPECT_FALSE(trigger->validate(false, false, false));
}

TEST_F(InputTriggerTest, StartupTriggerNotHeldDoesNotMatch)
{
    boot();
    auto trigger = make(false);
    EXPECT_FALSE(trigger->validate(false, false, false));
}

TEST_F(InputTriggerTest, StartupTriggerPressedJustAfterStartupReloads)
{
    boot();
    auto trigger = make(false);
    EXPECT_FALSE(trigger->validate(false, false, false));
    clock_ms::advance(500);
    button->set(true);
    EXPECT_TRUE(trigger->validate(false, false, false));
    EXPECT_EQ(fake_config::reload_count, 1);
    // Held, not pressed again
    EXPECT_TRUE(trigger->validate(false, false, false));
    EXPECT_EQ(fake_config::reload_count, 1);
}

TEST_F(InputTriggerTest, StartupTriggerPressedLaterDoesNothing)
{
    boot();
    auto trigger = make(false);
    trigger->validate(false, false, false);
    clock_ms::advance(5000);
    button->set(true);
    EXPECT_FALSE(trigger->validate(false, false, false));
    EXPECT_EQ(fake_config::reload_count, 0);
}

TEST_F(InputTriggerTest, AnyTimeTriggerReloadsOnEachPress)
{
    auto trigger = make(true);
    EXPECT_FALSE(trigger->validate(false, false, false));
    button->set(true);
    EXPECT_TRUE(trigger->validate(false, false, false));
    EXPECT_EQ(fake_config::reload_count, 1);
    clock_ms::advance(10000);
    EXPECT_TRUE(trigger->validate(false, false, false));
    EXPECT_EQ(fake_config::reload_count, 1);
    button->set(false);
    EXPECT_FALSE(trigger->validate(false, false, false));
    button->set(true);
    EXPECT_TRUE(trigger->validate(false, false, false));
    EXPECT_EQ(fake_config::reload_count, 2);
}

TEST_F(InputTriggerTest, AnyTimeTriggerAlreadyHeldWhenLoadedMatchesWithoutReloading)
{
    // After the reload a press caused, the button is usually still down: that mustn't reload again
    auto input = std::make_unique<TestInput>();
    input->set(true);
    InputActivationTrigger trigger(true, plain_trigger(), std::move(input), profile, 7, 2);
    EXPECT_TRUE(trigger.validate(false, false, false));
    EXPECT_EQ(fake_config::reload_count, 0);
}

TEST_F(InputTriggerTest, InvertedMatchesWhenReleased)
{
    auto config = plain_trigger();
    config.has_inverted = true;
    config.inverted = true;
    auto trigger = make(true, config);
    EXPECT_TRUE(trigger->validate(false, false, false));
    button->set(true);
    EXPECT_FALSE(trigger->validate(false, false, false));
}

TEST_F(InputTriggerTest, AnalogThresholdsMatchTheMappingTriggers)
{
    struct Case
    {
        AnalogToDigitalTriggerType type;
        int32_t value, max;
        uint16_t analog;
        bool expected;
    };
    const Case cases[] = {
        {AnalogToDigitalTriggerType_JoyHigh, 30000, 0, 30000, false},
        {AnalogToDigitalTriggerType_JoyHigh, 30000, 0, 30001, true},
        {AnalogToDigitalTriggerType_JoyLow, 30000, 0, 30000, false},
        {AnalogToDigitalTriggerType_JoyLow, 30000, 0, 29999, true},
        {AnalogToDigitalTriggerType_Exact, 1234, 0, 1234, true},
        {AnalogToDigitalTriggerType_Exact, 1234, 0, 1235, false},
        {AnalogToDigitalTriggerType_Range, 1000, 2000, 1000, false},
        {AnalogToDigitalTriggerType_Range, 1000, 2000, 1500, true},
        {AnalogToDigitalTriggerType_Range, 1000, 2000, 2000, false},
    };
    for (const auto &c : cases)
    {
        auto trigger = make(true, analog_trigger(c.type, c.value, c.max));
        // The digital state is ignored when there's an analog trigger
        button->digital = !c.expected;
        button->analog = c.analog;
        EXPECT_EQ(trigger->validate(false, false, false), c.expected) << c.type << " " << c.analog;
    }
}

TEST_F(InputTriggerTest, LinksItsDeviceWhenValidatingAndUnlinksOnReset)
{
    auto trigger = make(true);
    trigger->validate(false, false, false);
    trigger->validate(true, false, false);
    EXPECT_EQ(button->linked, 2);
    EXPECT_EQ(button->linked_claimed, 1);
    trigger->reset();
    EXPECT_EQ(button->linked, 3);
    EXPECT_EQ(button->linked_claimed, 1);
}

TEST_F(InputTriggerTest, ReportsDigitalChangesToTheTool)
{
    auto trigger = make(true);
    trigger->validate(false, false, true);
    EXPECT_TRUE(events_of(proto_Event_trigger_tag).empty());
    button->set(true);
    trigger->validate(false, false, true);
    trigger->validate(false, false, true);
    auto events = events_of(proto_Event_trigger_tag);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].event.trigger.id, 7u);
    EXPECT_EQ(events[0].event.trigger.listId, 2u);
    EXPECT_TRUE(events[0].event.trigger.state);
    // A full poll resends the current state
    trigger->validate(false, true, true);
    EXPECT_EQ(events_of(proto_Event_trigger_tag).size(), 2u);
}

TEST_F(InputTriggerTest, ReportsAnalogChangesToTheTool)
{
    auto trigger = make(true, analog_trigger(AnalogToDigitalTriggerType_JoyHigh, 30000));
    button->analog = 100;
    trigger->validate(false, false, true);
    button->analog = 40000;
    trigger->validate(false, false, true);
    trigger->validate(false, false, true);
    auto events = events_of(proto_Event_trigger_tag);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].event.trigger.stateRaw, 100u);
    EXPECT_FALSE(events[0].event.trigger.state);
    EXPECT_EQ(events[1].event.trigger.stateRaw, 40000u);
    EXPECT_TRUE(events[1].event.trigger.state);
}

TEST_F(InputTriggerTest, ListWithNoTriggersNeverMatches)
{
    ActivationTriggerList list;
    EXPECT_FALSE(list.validate(false, false, false));
    EXPECT_FALSE(list.validate(true, false, false));
    EXPECT_FALSE(list.claimed());
}

TEST_F(InputTriggerTest, ListNeedsEveryTrigger)
{
    bool first = true;
    bool second = false;
    ActivationTriggerList list;
    auto *a = new StubTrigger(profile, &first);
    auto *b = new StubTrigger(profile, &second);
    list.triggers.emplace_back(a);
    list.triggers.emplace_back(b);
    EXPECT_FALSE(list.validate(false, false, false));
    // Every trigger is still looked at (for the tool's per trigger state), and all reset after
    EXPECT_EQ(b->validations, 1);
    EXPECT_EQ(a->resets, 1);
    EXPECT_EQ(b->resets, 1);
    EXPECT_FALSE(list.validate(true, false, false));
    EXPECT_EQ(a->claims, 0);
    second = true;
    EXPECT_TRUE(list.validate(false, false, false));
    EXPECT_FALSE(list.claimed());
}

TEST_F(InputTriggerTest, ClaimingValidatesTwiceThenLatches)
{
    bool matches = true;
    ActivationTriggerList list;
    auto *trigger = new StubTrigger(profile, &matches);
    list.triggers.emplace_back(trigger);
    EXPECT_TRUE(list.validate(true, false, false));
    EXPECT_TRUE(list.claimed());
    EXPECT_EQ(trigger->claims, 1);
    // Once claimed the profile stays on these devices even when the trigger stops matching
    // (eg releasing a "Press Button Anytime" button)
    matches = false;
    EXPECT_TRUE(list.validate(true, false, false));
    EXPECT_EQ(trigger->claims, 1);
    // ...but the unclaimed check still tells the truth
    EXPECT_FALSE(list.validate(false, false, false));
}

TEST_F(InputTriggerTest, ListReportsItsStateToTheTool)
{
    bool matches = false;
    ActivationTriggerList list;
    list.list_id = 4;
    list.triggers.emplace_back(new StubTrigger(profile, &matches));
    list.validate(false, false, true);
    EXPECT_TRUE(events_of(proto_Event_activationList_tag).empty());
    matches = true;
    list.validate(false, false, true);
    list.validate(false, false, true);
    auto events = events_of(proto_Event_activationList_tag);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].event.activationList.listId, 4u);
    EXPECT_TRUE(events[0].event.activationList.state);
    list.validate(false, true, true);
    EXPECT_EQ(events_of(proto_Event_activationList_tag).size(), 2u);
}

TEST_F(InputTriggerTest, ListCombinesWhatItsTriggersAssign)
{
    bool matches = true;
    ActivationTriggerList list;
    auto *usb = new StubTrigger(profile, &matches, AssignUsb);
    auto *psx = new StubTrigger(profile, &matches, AssignPsx);
    list.triggers.emplace_back(usb);
    list.triggers.emplace_back(psx);
    EXPECT_EQ(list.assignedDevices(), AssignUsb | AssignPsx);
    ConsoleMode mode = ModeHid;
    EXPECT_FALSE(list.forcedConsoleMode(mode));
    const ConsoleMode ps3 = ModePs3;
    const ConsoleMode xb = ModeXbox360;
    usb->forced = &ps3;
    psx->forced = &xb;
    EXPECT_TRUE(list.forcedConsoleMode(mode));
    // The last trigger that forces a mode wins
    EXPECT_EQ(mode, ModeXbox360);
}

TEST_F(InputTriggerTest, AnyTimePressThroughAListReloadsOnce)
{
    // ProfileManager::update_device_assignments validates unclaimed, then claims: the trigger is
    // validated three times in that loop, and the press must only reload once
    ActivationTriggerList list;
    list.triggers.push_back(make(true));
    EXPECT_FALSE(list.validate(false, false, false));
    button->set(true);
    ASSERT_TRUE(list.validate(false, false, false));
    EXPECT_TRUE(list.validate(true, false, false));
    EXPECT_EQ(fake_config::reload_count, 1);
}
