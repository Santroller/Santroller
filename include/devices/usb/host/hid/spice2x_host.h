#pragma once
#include "devices/usb/host/hid/hid_host.h"
#include "protocols/spice2x.hpp"

class Spice2xHost : public HidHost {
public:
    Spice2xHost(uint8_t addr, uint8_t interface, uint16_t id) : HidHost(addr, interface, id) {
        m_subtype = Dancepad;
    }
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> device,
        tusb_desc_interface_t const *desc, uint16_t max_len, uint16_t vid, uint16_t pid,
        uint16_t revision, HID_ReportInfo_t *info);
    bool set_config() override;
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t bytes) override;
    bool tick_digital(proto_Output &output) override;
    uint16_t tick_analog(proto_Output &output) override { return 0; }
private:
    uint8_t m_ep_in = 0;
    uint16_t m_ep_in_size = 0;
    CFG_TUSB_MEM_ALIGN uint8_t m_buffer[64] = {};
    Spice2xInputReport m_report = {SPICE2X_INPUT_ID, 8, 0};
};
