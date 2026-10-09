#include <gtest/gtest.h>
#include <cstring>
#include "firmware_descriptors.hpp"
#include "struct_fields.hpp"

// The keyboard / mouse / media key device (HIDKeyboardDevice for KeyboardMouse profiles in ModeHid,
// src/emulation/usb/hid_keyboard_device.cpp), and its bluetooth twin (bt_descriptors.cpp, sent from
// BTGamepadDevice::process in bt_gamepad.cpp).
//
// Pairing, from the code that sends the reports: send_report(sizeof(keyboard), KEYBOARD_REPORT_ID,
// &keyboard) and the same for ConsumerReport / MouseReport, so the structs don't include the report id.
// set_report reads the lock lights from buffer[1] after the report id.

using namespace hid_desc;

namespace
{
Descriptor keyboard()
{
    auto d = parse(kHidKeyboard);
    EXPECT_TRUE(d.ok) << d.error;
    return d;
}
} // namespace

TEST(KeyboardDescriptor, ReportSizesMatchTheStructs)
{
    auto d = keyboard();
    EXPECT_EQ(d.report_bytes(ReportType::Input, KEYBOARD_REPORT_ID), sizeof(KeyboardReport));
    EXPECT_EQ(d.report_bytes(ReportType::Input, MOUSE_REPORT_ID), sizeof(MouseReport));
    EXPECT_EQ(d.report_bytes(ReportType::Input, CONSUMER_REPORT_ID), sizeof(ConsumerReport));
    // the lock lights: one byte after the id
    EXPECT_EQ(d.report_bytes(ReportType::Output, KEYBOARD_REPORT_ID), 1u);
    EXPECT_EQ(d.reports().size(), 4u);
}

TEST(KeyboardDescriptor, EndpointHoldsTheLargestReport)
{
    // config_descriptor() sizes the endpoint as sizeof(KeyboardReport) + 1
    auto d = keyboard();
    for (const auto &[report, bits] : d.report_bits)
    {
        if (report.first != ReportType::Input)
            continue;
        EXPECT_LE(d.wire_bytes(report.first, report.second), sizeof(KeyboardReport) + 1) << int(report.second);
    }
}

TEST(KeyboardDescriptor, KeyboardFields)
{
    auto d = keyboard();
    // modifiers: Left Control (0xE0) to Right GUI (0xE7), one bit each, in the modifier byte
    const FieldBits modifier = FIELD(KeyboardReport, modifier);
    for (uint16_t key = 0xE0; key <= 0xE7; key++)
    {
        const Element *e = d.find(ReportType::Input, KEYBOARD_REPORT_ID, usage(kPageKeyboard, key));
        ASSERT_NE(e, nullptr) << std::hex << key;
        EXPECT_EQ(e->bit_size, 1u);
        EXPECT_EQ(e->bit_offset, modifier.bit + (key - 0xE0));
    }
    // then the reserved byte
    const FieldBits reserved = FIELD(KeyboardReport, reserved);
    const Element *pad = d.element_at(ReportType::Input, KEYBOARD_REPORT_ID, reserved.bit);
    ASSERT_NE(pad, nullptr);
    EXPECT_TRUE(pad->constant());
    EXPECT_EQ(pad->bit_size, 8u);

    // then KEYBOARD_REPORT_KEYS keycodes, as an array of 0 - 255 keyboard usages
    const FieldBits keys = ARRAY_FIELD(KeyboardReport, keycode);
    const MainItem *array = nullptr;
    for (const auto &item : d.items)
        if (item.type == ReportType::Input && item.report_id == KEYBOARD_REPORT_ID && !(item.flags & kVariable))
            array = &item;
    ASSERT_NE(array, nullptr);
    EXPECT_EQ(array->bit_offset, keys.bit);
    EXPECT_EQ(array->report_size, 8u);
    EXPECT_EQ(array->report_count, unsigned(KEYBOARD_REPORT_KEYS));
    EXPECT_EQ(array->report_size * array->report_count, keys.width);
    EXPECT_TRUE(array->has_range);
    EXPECT_EQ(array->usage_min, usage(kPageKeyboard, 0));
    EXPECT_EQ(array->usage_max, usage(kPageKeyboard, 0xFF));
    EXPECT_EQ(array->logical_min, 0);
    EXPECT_EQ(array->logical_max, 0xFF);
}

TEST(KeyboardDescriptor, BootLayoutPrefix)
{
    // boot protocol (and BLE boot mode, which sends KEYBOARD_BOOT_REPORT_SIZE bytes of the same struct)
    // is modifier, reserved, six keys (HID 1.11 appendix B.1)
    EXPECT_EQ(offsetof(KeyboardReport, modifier), 0u);
    EXPECT_EQ(offsetof(KeyboardReport, reserved), 1u);
    EXPECT_EQ(offsetof(KeyboardReport, keycode), 2u);
    EXPECT_EQ(KEYBOARD_BOOT_REPORT_SIZE, 8);
    EXPECT_GE(sizeof(KeyboardReport), size_t(KEYBOARD_BOOT_REPORT_SIZE));
}

TEST(KeyboardDescriptor, LockLights)
{
    auto d = keyboard();
    // Num Lock, Caps Lock, Scroll Lock, Compose, Kana in bits 0 - 4 of the byte after the id
    for (uint16_t led = 1; led <= 5; led++)
    {
        const Element *e = d.find(ReportType::Output, KEYBOARD_REPORT_ID, usage(kPageLed, led));
        ASSERT_NE(e, nullptr) << led;
        EXPECT_EQ(e->bit_offset, led - 1u);
    }
}

TEST(KeyboardDescriptor, MouseFields)
{
    auto d = keyboard();
    // buttons 1 - 5 in the low bits of the buttons byte
    const FieldBits buttons = FIELD(MouseReport, buttons);
    for (uint16_t n = 1; n <= 5; n++)
    {
        const Element *e = d.find(ReportType::Input, MOUSE_REPORT_ID, button(n));
        ASSERT_NE(e, nullptr) << n;
        EXPECT_EQ(e->bit_offset, buttons.bit + n - 1);
    }
    // signed relative movement
    expect_field(d, ReportType::Input, MOUSE_REPORT_ID, desktop(kDesktopX), FIELD(MouseReport, x), false);
    expect_field(d, ReportType::Input, MOUSE_REPORT_ID, desktop(kDesktopY), FIELD(MouseReport, y), false);
    expect_field(d, ReportType::Input, MOUSE_REPORT_ID, desktop(kDesktopWheel), FIELD(MouseReport, wheel), false);
    expect_field(d, ReportType::Input, MOUSE_REPORT_ID, usage(kPageConsumer, 0x238), FIELD(MouseReport, pan), false);
    for (uint32_t u : {desktop(kDesktopX), desktop(kDesktopY), desktop(kDesktopWheel), usage(kPageConsumer, 0x238)})
        EXPECT_TRUE(d.find(ReportType::Input, MOUSE_REPORT_ID, u)->relative());
}

TEST(KeyboardDescriptor, ConsumerFields)
{
    auto d = keyboard();
    // CONSUMER_REPORT_KEYS 16 bit consumer usages, as an array
    const FieldBits usages = ARRAY_FIELD(ConsumerReport, usage);
    std::vector<const MainItem *> items;
    for (const auto &item : d.items)
        if (item.report_id == CONSUMER_REPORT_ID)
            items.push_back(&item);
    ASSERT_EQ(items.size(), 1u);
    const MainItem &array = *items[0];
    EXPECT_EQ(array.type, ReportType::Input);
    EXPECT_FALSE(array.flags & kVariable);
    EXPECT_EQ(array.bit_offset, usages.bit);
    EXPECT_EQ(array.report_size, 16u);
    EXPECT_EQ(array.report_count, unsigned(CONSUMER_REPORT_KEYS));
    EXPECT_EQ(array.usage_min, usage(kPageConsumer, 0));
    EXPECT_EQ(array.usage_max, usage(kPageConsumer, 0x3FF));
    EXPECT_EQ(array.logical_max, 0x3FF);
}

TEST(BtKeyboardDescriptor, SameLayoutAsUsb)
{
    EXPECT_EQ(std::vector<uint8_t>(desc_hid_report_keyboard, desc_hid_report_keyboard + desc_hid_report_keyboard_len),
              bytes_of(kHidKeyboard));
}
