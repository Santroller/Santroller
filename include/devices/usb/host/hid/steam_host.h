#pragma once
#include "devices/usb/host/hid/hid_host.h"
#include "protocols/steam_controller.hpp"

class SteamHost : public HidHost
{
public:
    ~SteamHost() {}
    SteamHost(uint8_t dev_addr, uint8_t interface, uint16_t id, uint16_t vid, uint16_t pid)
        : HidHost(dev_addr, interface, id), m_vid(vid), m_pid(pid)
    {
        m_subtype = SubType_Gamepad;
    }

    bool set_config() override;
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) override;
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list,
                                                   tusb_desc_interface_t const *itf_desc,
                                                   uint16_t max_len,
                                                   uint16_t vid,
                                                   uint16_t pid,
                                                   uint16_t revision,
                                                   HID_ReportInfo_t *info);
    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

private:
    void send_init_step();

    uint16_t m_vid = 0;
    uint16_t m_pid = 0;
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size = 0;
    uint8_t m_ep_out_size = 0;
    uint8_t m_init_step = 0;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64] = {};
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_out_buf[64] = {};
    SteamControllerState m_state = {};
};

