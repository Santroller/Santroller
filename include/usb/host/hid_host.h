#pragma once
#include "host.hpp"
#include "protocols/xinput.hpp"
#include "class/hid/hid.h"
#include "hidparser.h"
#include "protocols/dance_pad.hpp"

#define UP 1 << 0
#define DOWN 1 << 1
#define LEFT 1 << 2
#define RIGHT 1 << 3
class HidHost : public UsbHostInterface
{
public:
    ~HidHost() {}
    HidHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len);
    static const uint8_t dpad_bindings_reverse[8];
    uint32_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen, bool* status);
    uint32_t set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t bufsize, bool* status);
    virtual bool send_intr_report(const void *buffer, uint8_t len) {
        return false;
    }
};

class Ps3Host : public HidHost
{
public:
    ~Ps3Host() {}
    Ps3Host(uint8_t dev_addr, uint8_t interface, uint16_t id, bool third_party, bool rb2, bool ion, SubType subtype) : HidHost(dev_addr, interface, id), m_rb2(rb2), m_ion(ion), m_third_party(third_party)
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
    bool m_third_party;
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
    bool send_intr_report(const void *buffer, uint8_t len);
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(UsbButtonType type);
    uint16_t tick_analog(UsbAxisType type);
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    bool received_packet = false;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];

private:
    bool m_sensors_supported;
    bool m_lightbar_supported;
    bool m_vibration_supported;
    bool m_touchpad_supported;
    bool m_third_party;
};

class SwitchHost : public HidHost
{
public:
    ~SwitchHost() {}
    SwitchHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id) {}

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
class RaphnetHost : public HidHost
{
public:
    ~RaphnetHost() {}
    RaphnetHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id) {}

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
class GenericHost : public HidHost
{
public:
    ~GenericHost();
    GenericHost(uint8_t dev_addr, uint8_t interface, uint16_t id, HID_ReportInfo_t *info) : HidHost(dev_addr, interface, id), m_info(info) {}

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
    HID_ReportInfo_t *m_info;
    USB_Host_Data_t m_data;
};
class LTekHost : public HidHost
{
public:
    ~LTekHost() {}
    LTekHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id) {}

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(UsbButtonType type);
    uint16_t tick_analog(UsbAxisType type);

private:
    bool m_has_report_id;
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
};
class StepmaniaHost : public HidHost
{
public:
    ~StepmaniaHost() {}
    StepmaniaHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id) {}

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
    StepManiaX_Report_Data_t m_last_input_report;
};
class StreamDeckHost : public HidHost
{
public:
    ~StreamDeckHost() {}
    StreamDeckHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id) {}

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
class StadiaHost : public HidHost
{
public:
    ~StadiaHost() {}
    StadiaHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id) {}

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
    Stadia_Data_t m_last_input_report;
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
