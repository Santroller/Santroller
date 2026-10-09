#pragma once
#include <stdint.h>
#include <string.h>

// Keyboard and media key reports, read using the keyboard's own report descriptor. Report protocol lets
// keyboards report more than the 6 keys boot protocol allows, usually as one bit per key (NKRO).

#define HID_USAGE_PAGE_KEYBOARD_KEYS 0x07
#define HID_USAGE_PAGE_DESKTOP_KEYS 0x01
#define HID_USAGE_PAGE_CONSUMER_KEYS 0x0C
#define HID_USAGE_CONSUMER_CONTROL_APP 0x01
#define HID_USAGE_DESKTOP_KEYBOARD_APP 0x06
#define HID_USAGE_DESKTOP_JOYSTICK_APP 0x04
#define HID_USAGE_DESKTOP_GAMEPAD_APP 0x05
// array slots report this when too many keys are held to tell which ones are down
#define KEY_ERROR_ROLLOVER 0x01

class HidKeyboardDecoder
{
public:
    // consumer usages past this aren't media keys anyone binds
    static constexpr uint16_t MAX_CONSUMER_USAGE = 0x3FF;
    static constexpr uint8_t MAX_KEY_FIELDS = 24;

    // A run of keys in a report, either one bit per key (NKRO bitmaps, modifiers) or an array of
    // pressed keycodes (6KRO)
    struct KeyField
    {
        uint8_t report_id;
        // keyboard keys, or media keys from the consumer page
        bool consumer;
        bool variable;
        uint8_t size;
        uint8_t count;
        uint16_t bit_offset;
        uint16_t usage_min;
        uint16_t usage_max;
        int32_t logical_min;
    };

    // Find where the keyboard and media keys sit in the reports.
    // Returns true if this looks like a keyboard or media keys rather than something else that happens to have keys
    bool parse_report_descriptor(const uint8_t *desc, uint16_t len)
    {
        struct Globals
        {
            uint16_t usage_page;
            int32_t logical_min;
            uint8_t report_size;
            uint8_t report_count;
            uint8_t report_id;
        };
        Globals globals = {};
        Globals stack[4];
        uint8_t stack_depth = 0;
        // local usages, with the usage page in the top 16 bits
        uint32_t usages[MAX_USAGES];
        uint8_t usage_count = 0;
        uint32_t usage_min = 0;
        uint32_t usage_max = 0;
        // bit offset of the next input item in each report
        uint8_t report_ids[MAX_REPORT_IDS];
        uint16_t report_offsets[MAX_REPORT_IDS];
        uint8_t report_count = 0;
        bool keyboard_app = false;
        bool consumer_app = false;
        bool gamepad_app = false;
        m_field_count = 0;
        m_report_ids = false;

        uint16_t i = 0;
        while (i < len)
        {
            uint8_t prefix = desc[i++];
            if (prefix == 0xFE)
            {
                // long item, nothing we need is ever in one
                if (i >= len)
                {
                    break;
                }
                i += 2 + desc[i];
                continue;
            }
            uint8_t size = prefix & 3;
            if (size == 3)
            {
                size = 4;
            }
            if (i + size > len)
            {
                break;
            }
            uint32_t data = 0;
            for (uint8_t b = 0; b < size; b++)
            {
                data |= (uint32_t)desc[i + b] << (b * 8);
            }
            int32_t signed_data = data;
            if (size && size < 4 && (data & (1u << (size * 8 - 1))))
            {
                signed_data -= 1 << (size * 8);
            }
            i += size;
            uint8_t tag = prefix >> 4;
            switch ((prefix >> 2) & 3)
            {
            case 0: // main
            {
                if (tag == 0x8) // input
                {
                    uint16_t *offset = nullptr;
                    for (uint8_t r = 0; r < report_count; r++)
                    {
                        if (report_ids[r] == globals.report_id)
                        {
                            offset = &report_offsets[r];
                        }
                    }
                    if (!offset)
                    {
                        if (report_count == MAX_REPORT_IDS)
                        {
                            break;
                        }
                        report_ids[report_count] = globals.report_id;
                        report_offsets[report_count] = 0;
                        offset = &report_offsets[report_count++];
                    }
                    bool constant = data & 0x01;
                    bool variable = data & 0x02;
                    uint32_t min = usage_count ? usages[0] : usage_min;
                    uint32_t max = usage_count ? usages[usage_count - 1] : usage_max;
                    uint16_t page = min >> 16;
                    bool consumer = page == HID_USAGE_PAGE_CONSUMER_KEYS;
                    if (!constant && globals.report_size && globals.report_count && (page == HID_USAGE_PAGE_KEYBOARD_KEYS || consumer) && (max >> 16) == page)
                    {
                        uint16_t highest = consumer ? MAX_CONSUMER_USAGE : 0xFF;
                        KeyField field = {
                            .report_id = globals.report_id,
                            .consumer = consumer,
                            .variable = variable,
                            .size = globals.report_size,
                            .count = globals.report_count,
                            .bit_offset = *offset,
                            .usage_min = (uint16_t)(min & 0xFFFF),
                            .usage_max = (uint16_t)((max & 0xFFFF) < highest ? (max & 0xFFFF) : highest),
                            .logical_min = globals.logical_min};
                        if (variable && usage_count)
                        {
                            // a list of usages, one per bit, split into runs of consecutive keys
                            uint8_t start = 0;
                            for (uint8_t u = 1; u <= usage_count && u <= globals.report_count; u++)
                            {
                                if (u == usage_count || u == globals.report_count || usages[u] != usages[u - 1] + 1)
                                {
                                    field.usage_min = usages[start] & 0xFFFF;
                                    field.usage_max = usages[u - 1] & 0xFFFF;
                                    field.count = u - start;
                                    field.bit_offset = *offset + start * globals.report_size;
                                    if (field.usage_max <= highest)
                                    {
                                        add_field(field);
                                    }
                                    start = u;
                                }
                            }
                        }
                        else if (field.usage_min <= field.usage_max)
                        {
                            if (variable && field.count > field.usage_max - field.usage_min + 1)
                            {
                                field.count = field.usage_max - field.usage_min + 1;
                            }
                            add_field(field);
                        }
                    }
                    *offset += globals.report_size * globals.report_count;
                }
                else if (tag == 0xA && data == 0x01) // application collection
                {
                    uint32_t usage = usage_count ? usages[0] : usage_min;
                    if ((usage >> 16) == HID_USAGE_PAGE_DESKTOP_KEYS)
                    {
                        keyboard_app |= (usage & 0xFFFF) == HID_USAGE_DESKTOP_KEYBOARD_APP;
                        gamepad_app |= (usage & 0xFFFF) == HID_USAGE_DESKTOP_JOYSTICK_APP || (usage & 0xFFFF) == HID_USAGE_DESKTOP_GAMEPAD_APP;
                    }
                    consumer_app |= usage == ((HID_USAGE_PAGE_CONSUMER_KEYS << 16) | HID_USAGE_CONSUMER_CONTROL_APP);
                }
                usage_count = 0;
                usage_min = 0;
                usage_max = 0;
                break;
            }
            case 1: // global
                switch (tag)
                {
                case 0x0:
                    globals.usage_page = data;
                    break;
                case 0x1:
                    globals.logical_min = signed_data;
                    break;
                case 0x7:
                    globals.report_size = data;
                    break;
                case 0x8:
                    globals.report_id = data;
                    m_report_ids = true;
                    break;
                case 0x9:
                    globals.report_count = data;
                    break;
                case 0xA:
                    if (stack_depth < sizeof(stack) / sizeof(stack[0]))
                    {
                        stack[stack_depth++] = globals;
                    }
                    break;
                case 0xB:
                    if (stack_depth)
                    {
                        globals = stack[--stack_depth];
                    }
                    break;
                }
                break;
            case 2: // local
            {
                uint32_t usage = size == 4 ? data : ((uint32_t)globals.usage_page << 16) | data;
                switch (tag)
                {
                case 0x0:
                    if (usage_count < MAX_USAGES)
                    {
                        usages[usage_count++] = usage;
                    }
                    break;
                case 0x1:
                    usage_min = usage;
                    break;
                case 0x2:
                    usage_max = usage;
                    break;
                }
                break;
            }
            }
        }
        return m_field_count && (keyboard_app || consumer_app) && !gamepad_app;
    }

    // Boot protocol has a fixed layout: modifiers, reserved, then up to 6 keycodes
    void use_boot_layout()
    {
        m_report_ids = false;
        m_field_count = 0;
        add_field({.report_id = 0, .consumer = false, .variable = true, .size = 1, .count = 8, .bit_offset = 0, .usage_min = 0xE0, .usage_max = 0xE7, .logical_min = 0});
        add_field({.report_id = 0, .consumer = false, .variable = false, .size = 8, .count = 6, .bit_offset = 16, .usage_min = 0, .usage_max = 0xFF, .logical_min = 0});
    }

    // Update the held keys from an input report. Reports for other things (e.g. a mouse sharing the
    // interface) leave them alone, as does a keyboard reporting rollover.
    void handle_report(const uint8_t *report, uint16_t len)
    {
        uint8_t report_id = 0;
        if (m_report_ids && len)
        {
            report_id = *report++;
            len--;
        }
        bool rollover = false;
        bool matched = false;
        for (uint8_t f = 0; f < m_field_count; f++)
        {
            const KeyField &field = m_fields[f];
            if (field.report_id != report_id)
            {
                continue;
            }
            matched = true;
            for (uint8_t slot = 0; slot < field.count && !field.variable && !field.consumer; slot++)
            {
                int32_t value = read_bits(report, len, field.bit_offset + slot * field.size, field.size);
                rollover |= value - field.logical_min + field.usage_min == KEY_ERROR_ROLLOVER;
            }
        }
        // on rollover the keyboard can't tell which keys are held, so keep what we had
        if (!len || !matched || rollover)
        {
            return;
        }
        // build the new state on the side so held keys never read as released partway through
        uint32_t keys[sizeof(m_keys) / sizeof(m_keys[0])];
        uint32_t consumer[sizeof(m_consumer) / sizeof(m_consumer[0])];
        memcpy(keys, m_keys, sizeof(keys));
        memcpy(consumer, m_consumer, sizeof(consumer));
        for (uint8_t f = 0; f < m_field_count; f++)
        {
            const KeyField &field = m_fields[f];
            if (field.report_id != report_id)
            {
                continue;
            }
            uint32_t *state = field.consumer ? consumer : keys;
            uint16_t max = field.consumer ? MAX_CONSUMER_USAGE : 0xFF;
            uint16_t last = field.variable ? field.usage_min + field.count - 1 : field.usage_max;
            for (uint16_t key = field.usage_min; key <= last; key++)
            {
                set_key(state, max, key, false);
            }
        }
        for (uint8_t f = 0; f < m_field_count; f++)
        {
            const KeyField &field = m_fields[f];
            if (field.report_id != report_id)
            {
                continue;
            }
            uint32_t *state = field.consumer ? consumer : keys;
            uint16_t max = field.consumer ? MAX_CONSUMER_USAGE : 0xFF;
            for (uint8_t slot = 0; slot < field.count; slot++)
            {
                int32_t value = read_bits(report, len, field.bit_offset + slot * field.size, field.size);
                if (field.variable)
                {
                    if (value)
                    {
                        set_key(state, max, field.usage_min + slot, true);
                    }
                    continue;
                }
                int32_t key = value - field.logical_min + field.usage_min;
                // 0 is no key, and on keyboards 1 - 3 are error codes
                if (value >= field.logical_min && key > (field.consumer ? 0 : 3) && key <= field.usage_max)
                {
                    set_key(state, max, key, true);
                }
            }
        }
        // word at a time, so another core reading the keys never sees a torn update
        for (uint8_t w = 0; w < sizeof(keys) / sizeof(keys[0]); w++)
        {
            m_keys[w] = keys[w];
        }
        for (uint8_t w = 0; w < sizeof(consumer) / sizeof(consumer[0]); w++)
        {
            m_consumer[w] = consumer[w];
        }
    }

    bool key_pressed(uint8_t keycode) const { return m_keys[keycode >> 5] & (1u << (keycode & 31)); }
    bool consumer_pressed(uint16_t usage) const { return usage <= MAX_CONSUMER_USAGE && (m_consumer[usage >> 5] & (1u << (usage & 31))); }
    uint8_t field_count() const { return m_field_count; }
    const KeyField &field(uint8_t index) const { return m_fields[index]; }
    bool uses_report_ids() const { return m_report_ids; }

private:
    static constexpr uint8_t MAX_USAGES = 32;
    static constexpr uint8_t MAX_REPORT_IDS = 16;

    static uint32_t read_bits(const uint8_t *data, uint16_t len, uint32_t offset, uint8_t size)
    {
        uint32_t value = 0;
        for (uint8_t b = 0; b < size && b < 32; b++)
        {
            uint32_t bit = offset + b;
            if ((bit >> 3) >= len)
            {
                break;
            }
            if (data[bit >> 3] & (1 << (bit & 7)))
            {
                value |= 1u << b;
            }
        }
        return value;
    }

    static void set_key(uint32_t *keys, uint16_t max, uint16_t keycode, bool pressed)
    {
        if (keycode > max)
        {
            return;
        }
        if (pressed)
        {
            keys[keycode >> 5] |= 1u << (keycode & 31);
        }
        else
        {
            keys[keycode >> 5] &= ~(1u << (keycode & 31));
        }
    }

    void add_field(const KeyField &field)
    {
        if (m_field_count < MAX_KEY_FIELDS)
        {
            m_fields[m_field_count++] = field;
        }
    }

    KeyField m_fields[MAX_KEY_FIELDS];
    uint8_t m_field_count = 0;
    bool m_report_ids = false;
    // one bit per keycode, currently held
    uint32_t m_keys[8] = {0};
    // one bit per consumer usage, currently held
    uint32_t m_consumer[(MAX_CONSUMER_USAGE + 1) / 32] = {0};
};
