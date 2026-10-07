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
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(proto_Output& type);
    uint16_t tick_analog(proto_Output& type);
    bool key_pressed(uint8_t keycode) override;

private:
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
    // the last boot protocol report: modifiers, reserved, then up to 6 keycodes
    uint8_t m_keys[8] = {0};
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
