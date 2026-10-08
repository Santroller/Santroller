#include "tusb_option.h"
#include "devices/usb/host/hid/keyboard_mouse_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"

#define HID_USAGE_PAGE_KEYBOARD_KEYS 0x07
#define HID_USAGE_CONSUMER_CONTROL_APP 0x01
#define HID_USAGE_DESKTOP_KEYBOARD_APP 0x06
#define HID_USAGE_DESKTOP_JOYSTICK_APP 0x04
#define HID_USAGE_DESKTOP_GAMEPAD_APP 0x05
// array slots report this when too many keys are held to tell which ones are down
#define KEY_ERROR_ROLLOVER 0x01
#define MAX_USAGES 32
#define MAX_REPORT_IDS 16

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

void KeyboardHost::add_field(const KeyField &field)
{
    if (m_field_count < MAX_KEY_FIELDS)
    {
        m_fields[m_field_count++] = field;
    }
}

// Find where the keyboard and media keys sit in the reports. Report protocol lets keyboards report more
// than the 6 keys boot protocol allows, usually as one bit per key (NKRO).
// Returns true if this looks like a keyboard or media keys rather than something else that happens to have keys
bool KeyboardHost::parse_report_descriptor(const uint8_t *desc, uint16_t len)
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
                bool consumer = page == HID_USAGE_PAGE_CONSUMER;
                if (!constant && globals.report_size && globals.report_count && (page == HID_USAGE_PAGE_KEYBOARD_KEYS || consumer) && (max >> 16) == page)
                {
                    KeyField field = {
                        .report_id = globals.report_id,
                        .consumer = consumer,
                        .variable = variable,
                        .size = globals.report_size,
                        .count = globals.report_count,
                        .bit_offset = *offset,
                        .usage_min = (uint16_t)(min & 0xFFFF),
                        .usage_max = (uint16_t)tu_min32(max & 0xFFFF, consumer ? MAX_CONSUMER_USAGE : 0xFF),
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
                                if (field.usage_max <= (consumer ? MAX_CONSUMER_USAGE : 0xFF))
                                {
                                    add_field(field);
                                }
                                start = u;
                            }
                        }
                    }
                    else if (field.usage_min <= field.usage_max)
                    {
                        if (variable)
                        {
                            field.count = tu_min16(field.count, field.usage_max - field.usage_min + 1);
                        }
                        add_field(field);
                    }
                }
                *offset += globals.report_size * globals.report_count;
            }
            else if (tag == 0xA && data == 0x01) // application collection
            {
                uint32_t usage = usage_count ? usages[0] : usage_min;
                if ((usage >> 16) == HID_USAGE_PAGE_DESKTOP)
                {
                    keyboard_app |= (usage & 0xFFFF) == HID_USAGE_DESKTOP_KEYBOARD_APP;
                    gamepad_app |= (usage & 0xFFFF) == HID_USAGE_DESKTOP_JOYSTICK_APP || (usage & 0xFFFF) == HID_USAGE_DESKTOP_GAMEPAD_APP;
                }
                consumer_app |= usage == ((HID_USAGE_PAGE_CONSUMER << 16) | HID_USAGE_CONSUMER_CONTROL_APP);
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

std::shared_ptr<KeyboardHost> KeyboardHost::open_common(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc)
{
    uint8_t dev_addr = list->dev_addr();
    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    auto intf = std::make_shared<KeyboardHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id);
    intf->m_boot_subclass = itf_desc->bInterfaceSubClass == HID_SUBCLASS_BOOT;
    bool keyboard = intf->parse_report_descriptor(HidHost::s_report_desc, HidHost::s_report_desc_len);
    if (itf_desc->bInterfaceProtocol == HID_ITF_PROTOCOL_KEYBOARD)
    {
        if (!intf->m_field_count)
        {
            // couldn't make sense of the descriptor, boot protocol has a fixed layout:
            // modifiers, reserved, then up to 6 keycodes
            intf->m_boot_protocol = true;
            intf->m_report_ids = false;
            intf->m_field_count = 0;
            intf->add_field({.report_id = 0, .consumer = false, .variable = true, .size = 1, .count = 8, .bit_offset = 0, .usage_min = 0xE0, .usage_max = 0xE7, .logical_min = 0});
            intf->add_field({.report_id = 0, .consumer = false, .variable = false, .size = 8, .count = 6, .bit_offset = 16, .usage_min = 0, .usage_max = 0xFF, .logical_min = 0});
        }
    }
    else if (!keyboard)
    {
        return nullptr;
    }

    uint8_t endpoints = itf_desc->bNumEndpoints;
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep =
            (tusb_desc_endpoint_t const *)p_desc;
        TU_VERIFY(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);
        if (desc_ep->bEndpointAddress & 0x80)
        {
            intf->m_ep_in = desc_ep->bEndpointAddress;
            intf->m_ep_in_size = desc_ep->wMaxPacketSize;
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
        }
        else
        {
            intf->m_ep_out = desc_ep->bEndpointAddress;
            intf->m_ep_out_size = desc_ep->wMaxPacketSize;
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
        }
    }
    if (intf->m_ep_out)
    {
        list->host_devices_by_endpoint_out[intf->m_ep_out] = intf;
    }
    if (intf->m_ep_in)
    {
        list->host_devices_by_endpoint_in[intf->m_ep_in & (~0x80)] = intf;
    }
    // Keyboards with NKRO often send keys on a second interface instead of the boot one, and media keys
    // usually get their own interface, so only the first keyboard interface is assignable and it
    // reports the keys from all of them
    auto primary = list->keyboard.lock();
    if (primary)
    {
        primary->m_others.push_back(intf);
    }
    else
    {
        list->keyboard = intf;
        usb_host_add_assignable_interface(intf);
    }
    return intf;
}

std::shared_ptr<UsbHostInterface> KeyboardHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    if (itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_KEYBOARD)
    {
        return nullptr;
    }
    auto intf = open_common(list, itf_desc);
    if (intf)
    {
        USB_FreeReportInfo(info);
    }
    return intf;
}

std::shared_ptr<UsbHostInterface> KeyboardHost::open_report(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    if (itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_NONE)
    {
        return nullptr;
    }
    auto intf = open_common(list, itf_desc);
    if (intf)
    {
        USB_FreeReportInfo(info);
    }
    return intf;
}

bool KeyboardHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        const uint8_t *report = m_ep_in_buf;
        uint16_t len = xferred_bytes;
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
        if (result == XFER_RESULT_SUCCESS && len && matched && !rollover)
        {
            // build the new state on the side so held keys never read as released partway through
            uint32_t keys[8];
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
            for (uint8_t w = 0; w < 8; w++)
            {
                m_keys[w] = keys[w];
            }
            for (uint8_t w = 0; w < sizeof(consumer) / sizeof(consumer[0]); w++)
            {
                m_consumer[w] = consumer[w];
            }
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool KeyboardHost::key_pressed(uint8_t keycode)
{
    if (own_key_pressed(keycode))
    {
        return true;
    }
    for (auto &weak : m_others)
    {
        auto other = weak.lock();
        if (other && other->own_key_pressed(keycode))
        {
            return true;
        }
    }
    return false;
}

bool KeyboardHost::consumer_pressed(uint16_t usage)
{
    if (own_consumer_pressed(usage))
    {
        return true;
    }
    for (auto &weak : m_others)
    {
        auto other = weak.lock();
        if (other && other->own_consumer_pressed(usage))
        {
            return true;
        }
    }
    return false;
}

bool KeyboardHost::set_config()
{
    UsbHostInterface::set_config();
    if (m_boot_protocol || m_boot_subclass)
    {
        // Only boot interfaces support picking the protocol. Report protocol is the default, but a
        // previous host may have left it in boot protocol
        tusb_control_request_t set_protocol = {
            .bmRequestType = 0x21,
            .bRequest = HID_REQ_CONTROL_SET_PROTOCOL,
            .wValue = (uint16_t)(m_boot_protocol ? HID_PROTOCOL_BOOT : HID_PROTOCOL_REPORT),
            .wIndex = m_interface,
            .wLength = 0};
        send_ctrl_xfer(set_protocol, nullptr, nullptr);
    }
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}
// Inputs picked in the config tool as a USB button carry the key as the type
bool KeyboardHost::tick_digital(proto_Output& type)
{
    if (type.which_mapping == proto_Output_consumerKey_tag)
    {
        return consumer_pressed(type.mapping.consumerKey);
    }
    return type.which_mapping == proto_Output_keycode_tag && key_pressed(type.mapping.keycode);
}
uint16_t KeyboardHost::tick_analog(proto_Output& type)
{
    return tick_digital(type) ? UINT16_MAX : 0;
}
