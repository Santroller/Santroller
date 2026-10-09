#pragma once
#include "devices/usb/host/hid/hid_host.h"
#include "protocols/hid_keyboard.hpp"
#include "protocols/hid_mouse.hpp"

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
    static std::shared_ptr<KeyboardHost> open_common(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc);
    bool consumer_pressed(uint16_t usage);
    HidKeyboardDecoder m_decoder;
    bool m_boot_protocol = false;
    bool m_boot_subclass = false;
    // further keyboard interfaces on the same device, whose keys show up through this one
    std::vector<std::weak_ptr<KeyboardHost>> m_others;
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
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
    HID_ReportInfo_t *m_info;
    HidMouseDecoder m_decoder;
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
};
