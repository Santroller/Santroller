// LedMappingDevice effects in src/leds/base_led.cpp: drawn over what any mapping shows while it lights the
// LED, and redrawn every update so they keep moving for mappings the host only drives now and then.
#include "inputs_test_support.hpp"

namespace
{
// A single dimmable LED on a numbered pin, recording the last value written to it
class PinLedDevice : public LedMappingDevice
{
public:
    PinLedDevice(uint32_t pin, uint16_t *value) : m_pin(pin), m_value(value) {}
    void setup() override {}
    void off() override { *m_value = 0; }
    bool supports_brightness() override { return false; }
    uint8_t led_count() override { return 1; }
    LedId led_id(uint8_t) override
    {
        static const uint8_t pins = 0;
        return {&pins, m_pin};
    }
    int writes = 0;

protected:
    void write_val(uint16_t val) override
    {
        *m_value = val;
        writes++;
    }
    void write_val_raw(uint8_t, uint8_t r, uint8_t, uint8_t, uint8_t) override { *m_value = r * 257; }

private:
    uint32_t m_pin;
    uint16_t *m_value;
};

struct Rgb
{
    uint8_t r = 0, g = 0, b = 0;
};

// A strip of RGB LEDs going from black (or a released colour) to white, like RgbLedDevice
class StripLedDevice : public LedMappingDevice
{
public:
    StripLedDevice(std::vector<Rgb> *leds, bool has_start) : m_leds(leds), m_has_start(has_start) {}
    void setup() override {}
    void off() override { std::fill(m_leds->begin(), m_leds->end(), Rgb{}); }
    bool supports_brightness() override { return false; }
    uint8_t led_count() override { return m_leds->size(); }
    LedId led_id(uint8_t i) override { return {m_leds, i}; }

protected:
    bool is_rgb() override { return true; }
    bool colour(uint16_t val, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &w) override
    {
        // Released is dim red, pressed is white
        uint8_t level = val / 257;
        r = m_has_start ? std::max<uint8_t>(level, 40) : level;
        g = b = level;
        w = 255;
        return m_has_start || level;
    }
    void write_val(uint16_t val) override
    {
        for (uint8_t i = 0; i < m_leds->size(); i++)
        {
            write_val_at(i, val);
        }
    }
    void write_val_at(uint8_t i, uint16_t val) override
    {
        uint8_t r, g, b, w;
        if (colour(val, r, g, b, w))
        {
            write_val_raw(i, r, g, b, w);
        }
    }
    void write_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t) override { (*m_leds)[i] = {r, g, b}; }

private:
    std::vector<Rgb> *m_leds;
    bool m_has_start;
};

// Shows the rumble the host sets, like RumbleLedMapping, which only writes when the host sends something
class HostLedMapping : public LedMapping
{
public:
    using LedMapping::LedMapping;
    void update(bool, bool send_events) override { tick_test(send_events); }
    void reload() override {}
    void set_rumble(uint8_t left, uint8_t) override { m_device->set_val(left * 257); }
};

// Lit while its button is held, like InputLedMapping, which writes every update
class ButtonLedMapping : public LedMapping
{
public:
    using LedMapping::LedMapping;
    bool held = false;
    void update(bool, bool send_events) override
    {
        if (tick_test(send_events))
        {
            return;
        }
        if (!overridden())
        {
            m_device->set_val(held ? UINT16_MAX : 0);
        }
    }
    void reload() override {}
};

class LedEffectTest : public InputsTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile();
    uint16_t pin = 0;

    void SetUp() override
    {
        InputsTest::SetUp();
        // Start each test at the top of every effect's cycle
        clock_ms::set(1032 * 256 * 100);
    }
    template <typename Mapping>
    Mapping *add(std::unique_ptr<LedMappingDevice> device, LedEffectType effect, uint32_t speed = 20)
    {
        if (effect)
        {
            device->set_effect(effect, speed);
        }
        auto mapping = std::make_unique<Mapping>(std::move(device), profile, profile->leds.size());
        auto *ptr = mapping.get();
        profile->leds.push_back(std::move(mapping));
        return ptr;
    }
};
} // namespace

TEST_F(LedEffectTest, BreathingKeepsMovingWithoutTheHostSendingAnything)
{
    auto *host = add<HostLedMapping>(std::make_unique<PinLedDevice>(1, &pin), EffectBreathing);
    host->set_rumble(0xFF, 0);
    EXPECT_EQ(pin, UINT16_MAX);

    // A cycle is 1032ms at speed 20: off half way through, then back up
    clock_ms::advance(258);
    host->update(false, false);
    EXPECT_NEAR(pin, UINT16_MAX / 2, 257);
    clock_ms::advance(258);
    host->update(false, false);
    EXPECT_LE(pin, 257);
    clock_ms::advance(516);
    host->update(false, false);
    EXPECT_GE(pin, UINT16_MAX - 257);
}

TEST_F(LedEffectTest, BreathingScalesWhatTheMappingShows)
{
    auto *host = add<HostLedMapping>(std::make_unique<PinLedDevice>(1, &pin), EffectBreathing);
    host->set_rumble(0x80, 0);
    clock_ms::advance(258);
    host->update(false, false);
    EXPECT_NEAR(pin, 0x80 * 257 / 2, 257);
}

TEST_F(LedEffectTest, SlowerSpeedsTakeLonger)
{
    auto *host = add<HostLedMapping>(std::make_unique<PinLedDevice>(1, &pin), EffectBreathing, 1);
    host->set_rumble(0xFF, 0);
    // Speed 1 is 20 times slower than speed 20
    clock_ms::advance(258 * 20);
    host->update(false, false);
    EXPECT_NEAR(pin, UINT16_MAX / 2, 257);
}

TEST_F(LedEffectTest, BlinkTurnsTheLedOffAndOn)
{
    auto *button = add<ButtonLedMapping>(std::make_unique<PinLedDevice>(1, &pin), EffectBlink);
    button->held = true;
    button->update(false, false);
    EXPECT_EQ(pin, UINT16_MAX);
    // A blink is 100ms at speed 20
    clock_ms::advance(50);
    button->update(false, false);
    EXPECT_EQ(pin, 0);
    clock_ms::advance(50);
    button->update(false, false);
    EXPECT_EQ(pin, UINT16_MAX);
}

TEST_F(LedEffectTest, AnLedThatIsOffIsNotRedrawn)
{
    auto device = std::make_unique<PinLedDevice>(1, &pin);
    auto *dev = device.get();
    auto *host = add<HostLedMapping>(std::move(device), EffectBreathing);
    host->set_rumble(0, 0);
    int writes = dev->writes;
    clock_ms::advance(100);
    host->update(false, false);
    EXPECT_EQ(dev->writes, writes);
}

TEST_F(LedEffectTest, TheHostTakingTheLedStopsTheEffect)
{
    auto *button = add<ButtonLedMapping>(std::make_unique<PinLedDevice>(1, &pin), EffectBlink);
    auto *host = add<HostLedMapping>(std::make_unique<PinLedDevice>(1, &pin), (LedEffectType)0);
    button->held = true;
    button->update(false, false);
    host->begin_feedback();
    host->set_rumble(0x80, 0);
    host->end_feedback();
    clock_ms::advance(50);
    button->update(false, false);
    EXPECT_EQ(pin, 0x80 * 257);
}

TEST_F(LedEffectTest, TestingShowsTheLedWithoutTheEffect)
{
    auto *button = add<ButtonLedMapping>(std::make_unique<PinLedDevice>(1, &pin), EffectBlink);
    button->test(500);
    clock_ms::advance(50);
    button->update(false, false);
    EXPECT_EQ(pin, UINT16_MAX);
}

TEST_F(LedEffectTest, TurningTheMappingOffForgetsWhatItShowed)
{
    auto *host = add<HostLedMapping>(std::make_unique<PinLedDevice>(1, &pin), EffectBreathing);
    host->set_rumble(0xFF, 0);
    host->off();
    clock_ms::advance(100);
    host->update(false, false);
    EXPECT_EQ(pin, 0);
}

TEST_F(LedEffectTest, RgbBreathingFadesToTheReleasedColour)
{
    std::vector<Rgb> leds(1);
    auto *button = add<ButtonLedMapping>(std::make_unique<StripLedDevice>(&leds, true), EffectBreathing);
    button->held = true;
    clock_ms::advance(516);
    button->update(false, false);
    EXPECT_EQ(leds[0].r, 40);
    EXPECT_LE(leds[0].g, 1);
}

TEST_F(LedEffectTest, RgbBreathingWithoutAReleasedColourFadesToOff)
{
    std::vector<Rgb> leds(1);
    auto *button = add<ButtonLedMapping>(std::make_unique<StripLedDevice>(&leds, false), EffectBlink);
    button->held = true;
    button->update(false, false);
    EXPECT_EQ(leds[0].r, 255);
    // Off is drawn, rather than left showing white
    clock_ms::advance(50);
    button->update(false, false);
    EXPECT_EQ(leds[0].r, 0);
}

TEST_F(LedEffectTest, ReleasedColourShowsWithoutTheEffect)
{
    std::vector<Rgb> leds(1);
    auto *button = add<ButtonLedMapping>(std::make_unique<StripLedDevice>(&leds, true), EffectBlink);
    button->held = false;
    clock_ms::advance(50);
    button->update(false, false);
    EXPECT_EQ(leds[0].r, 40);
    EXPECT_EQ(leds[0].g, 0);
}

TEST_F(LedEffectTest, RainbowColoursEachLedAtTheMappingsBrightness)
{
    std::vector<Rgb> leds(3);
    auto *host = add<HostLedMapping>(std::make_unique<StripLedDevice>(&leds, false), EffectRainbow);
    host->set_rumble(0xFF, 0);
    // Each LED a third of the way further around the rainbow
    EXPECT_EQ(leds[0].r, 0);
    EXPECT_EQ(leds[0].b, 255);
    EXPECT_GT(leds[1].r, 200);
    EXPECT_EQ(leds[1].b, 0);
    EXPECT_GT(leds[2].g, 200);
    EXPECT_EQ(leds[2].r, 0);

    host->set_rumble(0x80, 0);
    EXPECT_EQ(leds[0].b, 0x80);

    // And it moves on its own
    clock_ms::advance(128);
    host->update(false, false);
    EXPECT_GT(leds[0].g, 0);
}
