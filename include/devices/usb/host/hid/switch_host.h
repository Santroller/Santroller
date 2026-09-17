#pragma once
#include "devices/usb/host/hid/hid_host.h"

#include "protocols/switch2.hpp"

bool switch_tick_digital(const uint8_t *buf, proto_Output &type);
uint16_t switch_tick_analog(const uint8_t *buf, proto_Output &type);

class SwitchHost : public HidHost
{
public:
    ~SwitchHost() {}
    SwitchHost(uint8_t dev_addr, uint8_t interface, uint16_t id, bool is_switch2 = false)
        : HidHost(dev_addr, interface, id), m_is_switch2(is_switch2)
    {
        m_subtype = SubType_Gamepad;
    }

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(proto_Output& type);
    uint16_t tick_analog(proto_Output& type);
    void set_rumble(uint8_t left, uint8_t right) override;
    void set_player_led(uint8_t player) override;
    bool has_rumble() const override { return true; }
    bool has_player_led() const override { return true; }

private:
    uint8_t m_rumble_left = 0;
    uint8_t m_rumble_right = 0;
    uint8_t m_packet_counter = 0;
    void send_handshake_step();

    bool m_is_switch2 = false;
    Switch2ControllerState m_switch2_state = {};
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size = 0;
    uint8_t m_ep_out_size = 0;
    uint8_t m_handshake_step = 0;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64] = {};
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_out_buf[64] = {};
};
