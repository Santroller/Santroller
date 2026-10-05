#pragma once
#include "devices/usb/host/hid/hid_host.h"
#include "protocols/ps5.hpp"
#include <atomic>

// Shared tick implementations — callable from both USB and BT hosts
bool ps5_tick_digital(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);
uint16_t ps5_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);
bool ps5_parse_capabilities(const uint8_t *data, uint16_t len,
                            SubType &subtype, bool &sensors, bool &lightbar, bool &vibration, bool &touchpad);

class Ps5Host : public HidHost
{
public:
    ~Ps5Host();
    Ps5Host(uint8_t dev_addr, uint8_t interface, uint16_t id) : HidHost(dev_addr, interface, id)
    {
        m_output_report.report_id = 0x02;
        m_output_report.vibration_flag = 1;
        m_output_report.light_bar_flag = 1;
        m_output_report.player_indicator_flag = 1;
    }

    bool set_config();
    bool send_intr_report(const void *buffer, uint8_t len);
    bool get_intr_report(void *buffer, uint8_t len);
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    void update(bool full_poll, bool send_events) override;
    void disconnect() override;
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(proto_Output& type);
    uint16_t tick_analog(proto_Output& type);
    void set_rumble(uint8_t left, uint8_t right) override;
    void set_player_led(uint8_t player) override;
    void set_lightbar(uint8_t r, uint8_t g, uint8_t b) override;
    bool has_rumble() const override { return true; }
    bool has_player_led() const override { return true; }
    bool has_lightbar() const override { return true; }

private:
    bool send_ps5_output();
    ps5_output_report m_output_report = {};
    bool m_output_dirty = true;
    bool m_out_pending = false;
    std::atomic<bool> m_out_done{false};
    xfer_result_t m_out_result = XFER_RESULT_SUCCESS;
    bool m_out_submit_failed = false;
    bool m_sensors_supported;
    bool m_lightbar_supported;
    bool m_vibration_supported;
    bool m_touchpad_supported;
    bool m_third_party;
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    bool received_packet = false;
    bool m_auth_registered = false;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_out_buf[64];
};
