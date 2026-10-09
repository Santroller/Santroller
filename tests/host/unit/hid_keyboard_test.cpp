#include <gtest/gtest.h>
#include <vector>
#include "protocols/hid_keyboard.hpp"

namespace
{
using Bytes = std::vector<uint8_t>;

// Standard boot keyboard: modifier bitmap, reserved byte, LED output, 6 keycode array
const Bytes kBootKeyboard = {
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01,
    0x75, 0x01, 0x95, 0x08, 0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
    0x95, 0x05, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02,
    0x95, 0x01, 0x75, 0x03, 0x91, 0x01,
    0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x05, 0x07, 0x19, 0x00, 0x2A, 0xFF, 0x00, 0x81, 0x00,
    0xC0};

// QMK style shared interface: a mouse (report 1) and an NKRO keyboard (report 6) with a modifier
// bitmap followed by one bit for each of keys 0x00 - 0xF7
const Bytes kQmkNkro = {
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x85, 0x01, 0x09, 0x01, 0xA1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x05, 0x15, 0x00, 0x25, 0x01, 0x95, 0x05, 0x75, 0x01, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x03, 0x81, 0x01,
    0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x81, 0x25, 0x7F, 0x95, 0x02, 0x75, 0x08, 0x81, 0x06,
    0xC0, 0xC0,
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, 0x06,
    0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01, 0x95, 0x08, 0x75, 0x01, 0x81, 0x02,
    0x05, 0x07, 0x19, 0x00, 0x29, 0xF7, 0x15, 0x00, 0x25, 0x01, 0x95, 0xF8, 0x75, 0x01, 0x81, 0x02,
    0xC0};

// QMK media keys: system control (report 3) then consumer control (report 4) as one 16 bit usage
const Bytes kQmkConsumer = {
    0x05, 0x01, 0x09, 0x80, 0xA1, 0x01, 0x85, 0x03, 0x19, 0x01, 0x2A, 0xB7, 0x00, 0x15, 0x01, 0x26, 0xB7, 0x00,
    0x95, 0x01, 0x75, 0x10, 0x81, 0x00, 0xC0,
    0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x04, 0x19, 0x01, 0x2A, 0xA0, 0x02, 0x15, 0x01, 0x26, 0xA0, 0x02,
    0x95, 0x01, 0x75, 0x10, 0x81, 0x00, 0xC0};

// Media keys as one bit each, listed individually and not all consecutive
const Bytes kConsumerBitmap = {
    0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08,
    0x09, 0xB5, 0x09, 0xB6, 0x09, 0xB7, 0x09, 0xCD, 0x09, 0xE2, 0x09, 0xE9, 0x09, 0xEA, 0x0A, 0x23, 0x02,
    0x81, 0x02, 0xC0};

// A gamepad whose home button sits on the consumer page
const Bytes kGamepadWithHome = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01,
    0x05, 0x0C, 0x0A, 0x23, 0x02, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x01, 0x81, 0x02,
    0x75, 0x07, 0x81, 0x01, 0xC0};

HidKeyboardDecoder parsed(const Bytes &desc, bool expect_keyboard = true)
{
    HidKeyboardDecoder decoder;
    EXPECT_EQ(decoder.parse_report_descriptor(desc.data(), desc.size()), expect_keyboard);
    return decoder;
}

void feed(HidKeyboardDecoder &decoder, const Bytes &report)
{
    decoder.handle_report(report.data(), report.size());
}

std::vector<int> held_keys(const HidKeyboardDecoder &decoder)
{
    std::vector<int> keys;
    for (int k = 0; k < 256; k++)
    {
        if (decoder.key_pressed(k))
        {
            keys.push_back(k);
        }
    }
    return keys;
}

std::vector<int> held_media(const HidKeyboardDecoder &decoder)
{
    std::vector<int> keys;
    for (int k = 0; k <= HidKeyboardDecoder::MAX_CONSUMER_USAGE; k++)
    {
        if (decoder.consumer_pressed(k))
        {
            keys.push_back(k);
        }
    }
    return keys;
}

// NKRO report 6: modifiers byte, then the key bitmap
Bytes nkro_report(uint8_t modifiers, std::initializer_list<uint8_t> keys)
{
    Bytes report(2 + 31, 0);
    report[0] = 6;
    report[1] = modifiers;
    for (uint8_t key : keys)
    {
        report[2 + key / 8] |= 1 << (key % 8);
    }
    return report;
}
} // namespace

TEST(HidKeyboardDescriptor, BootKeyboardHasModifiersAndKeyArray)
{
    auto decoder = parsed(kBootKeyboard);
    EXPECT_FALSE(decoder.uses_report_ids());
    ASSERT_EQ(decoder.field_count(), 2);
    const auto &modifiers = decoder.field(0);
    EXPECT_TRUE(modifiers.variable);
    EXPECT_EQ(modifiers.bit_offset, 0);
    EXPECT_EQ(modifiers.usage_min, 0xE0);
    EXPECT_EQ(modifiers.usage_max, 0xE7);
    const auto &keys = decoder.field(1);
    EXPECT_FALSE(keys.variable);
    // past the modifiers and the reserved byte; the LED output doesn't move input offsets
    EXPECT_EQ(keys.bit_offset, 16);
    EXPECT_EQ(keys.size, 8);
    EXPECT_EQ(keys.count, 6);
}

TEST(HidKeyboardDescriptor, NkroBitmapFollowsModifiersInItsReport)
{
    auto decoder = parsed(kQmkNkro);
    EXPECT_TRUE(decoder.uses_report_ids());
    ASSERT_EQ(decoder.field_count(), 2);
    EXPECT_EQ(decoder.field(1).report_id, 6);
    EXPECT_TRUE(decoder.field(1).variable);
    EXPECT_EQ(decoder.field(1).bit_offset, 8);
    EXPECT_EQ(decoder.field(1).count, 0xF8);
}

TEST(HidKeyboardDescriptor, ConsumerUsageListSplitsIntoRuns)
{
    auto decoder = parsed(kConsumerBitmap);
    ASSERT_EQ(decoder.field_count(), 5);
    EXPECT_EQ(decoder.field(0).usage_min, 0xB5);
    EXPECT_EQ(decoder.field(0).count, 3);
    EXPECT_EQ(decoder.field(4).usage_min, 0x223);
    EXPECT_EQ(decoder.field(4).bit_offset, 7);
    for (uint8_t f = 0; f < decoder.field_count(); f++)
    {
        EXPECT_TRUE(decoder.field(f).consumer);
    }
}

TEST(HidKeyboardDescriptor, GamepadWithConsumerButtonIsNotAKeyboard)
{
    parsed(kGamepadWithHome, false);
}

TEST(HidKeyboardDescriptor, KeysOutsideAKeyboardCollectionAreNotAKeyboard)
{
    // just the key array, with no application collection around it
    const Bytes desc = {0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x05, 0x07, 0x19, 0x00, 0x2A, 0xFF, 0x00, 0x81, 0x00};
    auto decoder = parsed(desc, false);
    EXPECT_EQ(decoder.field_count(), 1);
}

TEST(HidKeyboardDescriptor, PushAndPopRestoreGlobals)
{
    const Bytes desc = {
        0x05, 0x01, 0x09, 0x06, 0xA1, 0x01,
        0x75, 0x08, 0x95, 0x01,
        0xA4,                   // push
        0x75, 0x01, 0x95, 0x10, // 16 one bit items
        0x05, 0x07, 0x19, 0x04, 0x29, 0x13, 0x81, 0x02,
        0xB4,                   // pop back to one 8 bit item
        0x05, 0x07, 0x19, 0x00, 0x29, 0xFF, 0x81, 0x00,
        0xC0};
    auto decoder = parsed(desc);
    ASSERT_EQ(decoder.field_count(), 2);
    EXPECT_EQ(decoder.field(1).size, 8);
    EXPECT_EQ(decoder.field(1).count, 1);
    EXPECT_EQ(decoder.field(1).bit_offset, 16);
}

TEST(HidKeyboardDescriptor, TruncatedDescriptorStopsCleanly)
{
    for (size_t len = 0; len < kQmkNkro.size(); len++)
    {
        HidKeyboardDecoder decoder;
        decoder.parse_report_descriptor(kQmkNkro.data(), len);
    }
}

TEST(HidKeyboardDescriptor, LongItemsAreSkipped)
{
    Bytes desc = {0xFE, 0x02, 0x00, 0xAA, 0xBB};
    desc.insert(desc.end(), kBootKeyboard.begin(), kBootKeyboard.end());
    auto decoder = parsed(desc);
    EXPECT_EQ(decoder.field_count(), 2);
}

TEST(HidKeyboardReport, BootReportPressesModifiersAndKeys)
{
    auto decoder = parsed(kBootKeyboard);
    feed(decoder, {0x02, 0x00, 0x04, 0x05, 0, 0, 0, 0});
    EXPECT_EQ(held_keys(decoder), (std::vector<int>{0x04, 0x05, 0xE1}));
    feed(decoder, {0, 0, 0, 0, 0, 0, 0, 0});
    EXPECT_TRUE(held_keys(decoder).empty());
}

TEST(HidKeyboardReport, RolloverKeepsWhatWasHeld)
{
    auto decoder = parsed(kBootKeyboard);
    feed(decoder, {0x02, 0x00, 0x04, 0x05, 0, 0, 0, 0});
    feed(decoder, {0x02, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01});
    EXPECT_EQ(held_keys(decoder), (std::vector<int>{0x04, 0x05, 0xE1}));
}

TEST(HidKeyboardReport, ErrorCodesAreNotKeys)
{
    auto decoder = parsed(kBootKeyboard);
    feed(decoder, {0x00, 0x00, 0x02, 0x03, 0x06, 0, 0, 0});
    EXPECT_EQ(held_keys(decoder), (std::vector<int>{0x06}));
}

TEST(HidKeyboardReport, ShortReportReleasesTheMissingKeys)
{
    auto decoder = parsed(kBootKeyboard);
    feed(decoder, {0x00, 0x00, 0x04, 0x05, 0x06, 0, 0, 0});
    feed(decoder, {0x00, 0x00, 0x04});
    EXPECT_EQ(held_keys(decoder), (std::vector<int>{0x04}));
}

TEST(HidKeyboardReport, EmptyReportIsIgnored)
{
    auto decoder = parsed(kBootKeyboard);
    feed(decoder, {0x00, 0x00, 0x04, 0, 0, 0, 0, 0});
    decoder.handle_report(nullptr, 0);
    EXPECT_EQ(held_keys(decoder), (std::vector<int>{0x04}));
}

TEST(HidKeyboardReport, NkroReportsMoreThanSixKeys)
{
    auto decoder = parsed(kQmkNkro);
    feed(decoder, nkro_report(0x01, {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x2C}));
    EXPECT_EQ(held_keys(decoder), (std::vector<int>{0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x2C, 0xE0}));
    feed(decoder, nkro_report(0x00, {0x0D}));
    EXPECT_EQ(held_keys(decoder), (std::vector<int>{0x0D}));
}

TEST(HidKeyboardReport, OtherReportsOnTheInterfaceLeaveKeysAlone)
{
    auto decoder = parsed(kQmkNkro);
    feed(decoder, nkro_report(0x00, {0x04}));
    feed(decoder, {0x01, 0x01, 0x05, 0x05}); // mouse report
    feed(decoder, {0x09, 0x00});             // a report id nothing uses
    EXPECT_EQ(held_keys(decoder), (std::vector<int>{0x04}));
}

TEST(HidKeyboardReport, BootLayoutFallback)
{
    HidKeyboardDecoder decoder;
    decoder.use_boot_layout();
    feed(decoder, {0x10, 0x00, 0x29, 0, 0, 0, 0, 0});
    EXPECT_EQ(held_keys(decoder), (std::vector<int>{0x29, 0xE4}));
}

TEST(HidKeyboardMedia, QmkConsumerUsageIsHeldUntilReleased)
{
    auto decoder = parsed(kQmkConsumer);
    feed(decoder, {0x04, 0xE9, 0x00});
    EXPECT_EQ(held_media(decoder), (std::vector<int>{0xE9}));
    // system control (sleep) isn't a media key
    feed(decoder, {0x03, 0x82, 0x00});
    EXPECT_EQ(held_media(decoder), (std::vector<int>{0xE9}));
    feed(decoder, {0x04, 0x00, 0x00});
    EXPECT_TRUE(held_media(decoder).empty());
}

TEST(HidKeyboardMedia, BitmapUsagesMapToTheirBits)
{
    auto decoder = parsed(kConsumerBitmap);
    feed(decoder, {0x88}); // bit 3 = play / pause, bit 7 = browser home
    EXPECT_EQ(held_media(decoder), (std::vector<int>{0xCD, 0x223}));
    feed(decoder, {0x01});
    EXPECT_EQ(held_media(decoder), (std::vector<int>{0xB5}));
    EXPECT_TRUE(held_keys(decoder).empty());
}

TEST(HidKeyboardMedia, UsagesPastTheLimitAreIgnored)
{
    EXPECT_FALSE(HidKeyboardDecoder().consumer_pressed(HidKeyboardDecoder::MAX_CONSUMER_USAGE + 1));
}
