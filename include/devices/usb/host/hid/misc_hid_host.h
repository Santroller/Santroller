#pragma once
#include "devices/usb/host/hid/hid_host.h"
#include "protocols/controller_reports.hpp"
#include "protocols/stadia.hpp"
#include <atomic>

class StreamDeckHost : public HidHost
{
public:
    ~StreamDeckHost() {}
    StreamDeckHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id) {}

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(proto_Output &type);
    uint16_t tick_analog(proto_Output &type);

private:
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    // Size of the header before the key states (1 on the original / Mini, 4 on newer models)
    uint8_t m_key_offset = 1;
    USB_Host_Data_t m_data = {};
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
};

class StadiaHost : public HidHost
{
public:
    ~StadiaHost() {}
    StadiaHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id) {}

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    void update(bool full_poll, bool send_events) override;
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(proto_Output &type);
    uint16_t tick_analog(proto_Output &type);
    void set_rumble(uint8_t left, uint8_t right) override { m_rumble.set(left, right); }
    bool has_rumble() const override { return true; }

private:
    bool send_rumble();
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
    Stadia_Data_t m_last_input_report = {};
    StadiaRumble m_rumble;
    bool m_out_pending = false;
    std::atomic<bool> m_out_done{false};
    xfer_result_t m_out_result = XFER_RESULT_SUCCESS;
    bool m_out_submit_failed = false;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_out_buf[1 + STADIA_RUMBLE_LEN];
};
