#pragma once
#include "devices/usb/host/host.hpp"
#include "protocols/switch2.hpp"

// The vendor specific interface a Switch 2 controller takes commands on over USB. It sends the
// controller SWITCH2_USB_INIT once configured, which turns motion on and reads the gyro bias for the
// controller's HID interface, where its inputs arrive. It isn't a controller itself.
class Switch2CommandHost : public UsbHostInterface
{
public:
    Switch2CommandHost(uint8_t dev_addr, uint8_t interface, uint16_t id, std::weak_ptr<UsbHostInterface> hid);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len);
    bool set_config() override;
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) override;
    // nothing for the config tool to list
    void update(bool full_poll, bool send_events) override { still_connected = true; }
    bool is_assignable() const override { return false; }
    bool tick_digital(proto_Output &type) override { return false; }
    uint16_t tick_analog(proto_Output &type) override { return 0; }

private:
    void send_next();
    std::weak_ptr<UsbHostInterface> m_hid;
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    // the command in SWITCH2_USB_INIT waiting for its reply
    uint8_t m_step = 0;
    CFG_TUSB_MEM_ALIGN uint8_t m_out_buf[64];
    CFG_TUSB_MEM_ALIGN uint8_t m_in_buf[SWITCH2_FLASH_REPLY_LEN];
};
