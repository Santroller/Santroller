#pragma once
#include "devices/usb/host/hid/hid_host.h"

class Ps4Host : public HidHost
{
public:
    ~Ps4Host();
    Ps4Host(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id) {}

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    void disconnect() override;
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(proto_Output& type);
    uint16_t tick_analog(proto_Output& type);

private:
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    bool m_sensors_supported;
    bool m_lightbar_supported;
    bool m_vibration_supported;
    bool m_touchpad_supported;
    bool m_third_party;
    bool m_auth_registered = false;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
};
