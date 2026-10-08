#pragma once
#include "devices/usb/host/hid/hid_host.h"
#include "protocols/xbox_hid.hpp"
#include <atomic>

// Xbox One S / Series controllers that enumerate as a HID device instead of using GIP
class XboxHidHost : public HidHost
{
public:
    ~XboxHidHost() {}
    XboxHidHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id) {}

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    void update(bool full_poll, bool send_events) override;
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(proto_Output& type);
    uint16_t tick_analog(proto_Output& type);
    void set_rumble(uint8_t left, uint8_t right) override { m_rumble.set(left, right); }
    bool has_rumble() const override { return true; }

private:
    bool send_rumble();
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    uint16_t m_pid = 0;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
    XboxHidState m_state = {};
    XboxHidDescriptor m_desc;
    XboxHidRumble m_rumble;
    bool m_out_pending = false;
    std::atomic<bool> m_out_done{false};
    xfer_result_t m_out_result = XFER_RESULT_SUCCESS;
    bool m_out_submit_failed = false;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_out_buf[1 + XBOX_HID_RUMBLE_LEN];
};
