// LedMapping in src/leds/led_mapping.cpp: once the host changes an LED through a feedback command,
// mappings driven by inputs leave that LED alone for COMMAND_HOLD_MS, as the host takes precedence.
#include "inputs_test_support.hpp"

namespace
{
// A single LED on a numbered pin, recording the last value written to it
class PinLedDevice : public LedMappingDevice
{
public:
    PinLedDevice(uint32_t pin, uint16_t *value) : m_pin(pin), m_value(value) {}
    void setup() override {}
    void off() override {}
    bool supports_brightness() override { return false; }
    uint8_t led_count() override { return 1; }
    LedId led_id(uint8_t) override
    {
        static const uint8_t pins = 0;
        return {&pins, m_pin};
    }

protected:
    void write_val(uint16_t val) override { *m_value = val; }
    void write_val_raw(uint8_t, uint8_t r, uint8_t, uint8_t, uint8_t) override { *m_value = r; }

private:
    uint32_t m_pin;
    uint16_t *m_value;
};

// Shows the rumble the host sets, like RumbleLedMapping
class HostLedMapping : public LedMapping
{
public:
    using LedMapping::LedMapping;
    void update(bool, bool) override {}
    void reload() override {}
    void set_rumble(uint8_t left, uint8_t) override { m_device->set_val(left * 257); }
};

// Lit while its button is held, like InputLedMapping
class ButtonLedMapping : public LedMapping
{
public:
    using LedMapping::LedMapping;
    bool held = false;
    void update(bool, bool) override
    {
        if (!overridden())
        {
            m_device->set_val(held ? UINT16_MAX : 0);
        }
    }
    void reload() override {}
};

class LedOverrideTest : public InputsTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile();
    uint16_t pin1 = 0;
    uint16_t pin2 = 0;
    HostLedMapping *host = nullptr;
    ButtonLedMapping *button = nullptr;

    void SetUp() override
    {
        InputsTest::SetUp();
        auto h = std::make_unique<HostLedMapping>(std::make_unique<PinLedDevice>(1, &pin1), profile, 0);
        auto b = std::make_unique<ButtonLedMapping>(std::make_unique<PinLedDevice>(1, &pin1), profile, 1);
        host = h.get();
        button = b.get();
        profile->leds.push_back(std::move(h));
        profile->leds.push_back(std::move(b));
    }
    // What Instance::update_feedback does for each LED mapping
    void feedback(uint8_t rumble)
    {
        for (auto &led : profile->leds)
        {
            led->begin_feedback();
            led->set_rumble(rumble, 0);
            led->end_feedback();
        }
    }
};
} // namespace

TEST_F(LedOverrideTest, ButtonDrivesTheLedWithoutCommands)
{
    button->held = true;
    button->update(false, false);
    EXPECT_EQ(pin1, UINT16_MAX);
}

TEST_F(LedOverrideTest, CommandTakesPrecedenceThenHandsBack)
{
    feedback(0x80);
    EXPECT_EQ(pin1, 0x80 * 257);
    button->held = true;
    button->update(false, false);
    EXPECT_EQ(pin1, 0x80 * 257);

    clock_ms::advance(LedMapping::COMMAND_HOLD_MS - 1);
    button->update(false, false);
    EXPECT_EQ(pin1, 0x80 * 257);

    clock_ms::advance(1);
    button->update(false, false);
    EXPECT_EQ(pin1, UINT16_MAX);
}

TEST_F(LedOverrideTest, TurningTheLedOffIsAlsoACommand)
{
    feedback(0x80);
    clock_ms::advance(LedMapping::COMMAND_HOLD_MS);
    feedback(0);
    button->held = true;
    button->update(false, false);
    EXPECT_EQ(pin1, 0);
}

TEST_F(LedOverrideTest, RepeatingTheSameCommandDoesNotExtendTheHold)
{
    feedback(0x80);
    clock_ms::advance(LedMapping::COMMAND_HOLD_MS);
    feedback(0x80);
    button->held = true;
    button->update(false, false);
    EXPECT_EQ(pin1, UINT16_MAX);
}

TEST_F(LedOverrideTest, StartingOffIsNotACommand)
{
    feedback(0);
    button->held = true;
    button->update(false, false);
    EXPECT_EQ(pin1, UINT16_MAX);
}

TEST_F(LedOverrideTest, OtherLedsAreUnaffected)
{
    auto b = std::make_unique<ButtonLedMapping>(std::make_unique<PinLedDevice>(2, &pin2), profile, 2);
    auto *other = b.get();
    profile->leds.push_back(std::move(b));
    feedback(0x80);
    other->held = true;
    other->update(false, false);
    EXPECT_EQ(pin2, UINT16_MAX);
}
