#pragma once
#include "host.hpp"
#include "protocols/xinput.hpp"
#include "hidparser.h"

class HidHost : public UsbHostInterface
{
public:
    ~HidHost() {}
    HidHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len);
};

class Ps3Host : public HidHost
{
public:
    ~Ps3Host() {}
    Ps3Host(uint8_t dev_addr, uint8_t interface, uint16_t id, bool rb2, bool ion, SubType subtype) : HidHost(dev_addr, interface, id), m_rb2(rb2), m_ion(ion)
    {
        m_subtype = subtype;
    }

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(UsbButtonType type);
    uint16_t tick_analog(UsbAxisType type);

private:
    bool m_rb2;
    bool m_ion;
    bool m_wt;
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[sizeof(XInputGamepad_Data_t)];
};

class Ps4Host : public HidHost
{
public:
    ~Ps4Host() {}
    Ps4Host(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id) {}

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(UsbButtonType type);
    uint16_t tick_analog(UsbAxisType type);

private:
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    bool m_sensors_supported;
    bool m_lightbar_supported;
    bool m_vibration_supported;
    bool m_touchpad_supported;
    bool m_third_party;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
};

class Ps5Host : public HidHost
{
public:
    ~Ps5Host() {}
    Ps5Host(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id) {}

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(UsbButtonType type);
    uint16_t tick_analog(UsbAxisType type);

private:
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    bool m_sensors_supported;
    bool m_lightbar_supported;
    bool m_vibration_supported;
    bool m_touchpad_supported;
    bool m_third_party;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[sizeof(XInputGamepad_Data_t)];
};
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
    bool tick_digital(UsbButtonType type);
    uint16_t tick_analog(UsbAxisType type);

private:
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
};

class MouseHost : public HidHost
{
public:
    ~MouseHost() {}
    MouseHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id)
    {
        m_subtype = KeyboardMouse;
    }

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(UsbButtonType type);
    uint16_t tick_analog(UsbAxisType type);

private:
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
};
