#pragma once
#include "devices/usb/host/hid/hid_host.h"
#include "utils.h"

// Shared tick implementations — callable from both USB and BT hosts
bool ps3_tick_digital(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type, bool wt = false);
uint16_t ps3_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);

class Ps3Host : public HidHost
{
public:
    ~Ps3Host() {}
    Ps3Host(uint8_t dev_addr, uint8_t interface, uint16_t id, bool third_party, bool rb2, bool ion, bool wt, SubType subtype) : HidHost(dev_addr, interface, id), m_rb2(rb2), m_ion(ion), m_wt(wt), m_third_party(third_party), m_init_time(millis())
    {
        m_subtype = subtype;
    }

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(proto_Output& type);
    uint16_t tick_analog(proto_Output& type);
    void set_rumble(uint8_t left, uint8_t right) override;
    void set_player_led(uint8_t player) override;
    void set_euphoria_led(bool state) override;
    bool has_rumble() const override { return true; }
    bool has_player_led() const override { return true; }
    bool has_euphoria_led() const override { return m_subtype == DjHeroTurntable; }

private:
    bool send_ps3_output();
    bool m_rb2;
    bool m_ion;
    bool m_wt;
    bool m_third_party;
    uint32_t m_init_time = 0;
    uint32_t m_last_ghl_poke = 0;
    uint8_t m_rumble_left = 0;
    uint8_t m_rumble_right = 0;
    uint8_t m_player = 0;
    bool m_euphoria = false;
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
};
