#pragma once
#include "devices/usb/host/hid/hid_host.h"

class KeyboardHost : public HidHost
{
public:
    ~KeyboardHost() {}
    KeyboardHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id)
    {
        m_subtype = KeyboardMouse;
    }

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    // boot keyboards
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    // keyboards that only exist in report protocol, such as the extra NKRO interface many keyboards
    // have, and media key interfaces
    static std::shared_ptr<UsbHostInterface> open_report(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(proto_Output& type);
    uint16_t tick_analog(proto_Output& type);
    bool key_pressed(uint8_t keycode) override;

private:
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
    static constexpr uint8_t MAX_KEY_FIELDS = 24;
    // consumer usages past this aren't media keys anyone binds
    static constexpr uint16_t MAX_CONSUMER_USAGE = 0x3FF;
    static std::shared_ptr<KeyboardHost> open_common(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc);
    bool parse_report_descriptor(const uint8_t *desc, uint16_t len);
    void add_field(const KeyField &field);
    bool own_key_pressed(uint8_t keycode) const { return m_keys[keycode >> 5] & (1u << (keycode & 31)); }
    bool own_consumer_pressed(uint16_t usage) const { return usage <= MAX_CONSUMER_USAGE && (m_consumer[usage >> 5] & (1u << (usage & 31))); }
    bool consumer_pressed(uint16_t usage);
    KeyField m_fields[MAX_KEY_FIELDS];
    uint8_t m_field_count = 0;
    bool m_report_ids = false;
    bool m_boot_protocol = false;
    bool m_boot_subclass = false;
    // further keyboard interfaces on the same device, whose keys show up through this one
    std::vector<std::weak_ptr<KeyboardHost>> m_others;
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
    // one bit per keycode, currently held
    uint32_t m_keys[8] = {0};
    // one bit per consumer usage, currently held
    uint32_t m_consumer[(MAX_CONSUMER_USAGE + 1) / 32] = {0};
};

class MouseHost : public HidHost
{
public:
    ~MouseHost();
    MouseHost(uint8_t dev_addr, uint8_t interface, uint16_t id, HID_ReportInfo_t *info);

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    void disconnect() override;
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(proto_Output& type);
    uint16_t tick_analog(proto_Output& type);
    bool mouse_button(MouseButtonType button) override;
    uint16_t mouse_axis(MouseAxisType axis) override;

private:
    void find_items();
    // The mouse's report layout, parsed from its report descriptor (report protocol, so the
    // wheel is available, unlike in boot protocol)
    HID_ReportInfo_t *m_info;
    HID_ReportItem_t *m_axis_items[4] = {nullptr};
    HID_ReportItem_t *m_button_items[3] = {nullptr};
    // latest movement, and when it arrived (mice only send reports while something changes)
    int32_t m_movement[4] = {0};
    uint8_t m_buttons = 0;
    uint32_t m_last_report_us = 0;
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
};
