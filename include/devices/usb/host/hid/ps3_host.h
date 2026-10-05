#pragma once
#include "devices/usb/host/hid/hid_host.h"
#include "protocols/ps3.hpp"
#include "protocols/switch_arcade.hpp"
#include "utils.h"

// Shared tick implementations — callable from both USB and BT hosts
bool ps3_tick_digital(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type, bool wt = false);
uint16_t ps3_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);
uint16_t ps3_tick_button_pressure(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);

class Ps3Host : public HidHost
{
public:
    ~Ps3Host() {}
    Ps3Host(uint8_t dev_addr, uint8_t interface, uint16_t id, bool third_party, bool rb2, bool ion, bool wt, SubType subtype) : HidHost(dev_addr, interface, id), m_rb2(rb2), m_ion(ion), m_wt(wt), m_third_party(third_party)
    {
        m_subtype = subtype;
        memset(m_ep_in_buf, 0, sizeof(m_ep_in_buf));
        auto *input_report = reinterpret_cast<PS3Dpad_Data_t *>(m_ep_in_buf);
        input_report->dpad = 8;
        input_report->leftStickX = 0x80;
        input_report->leftStickY = 0x80;
        input_report->rightStickX = 0x80;
        input_report->rightStickY = 0x80;
        m_output_report.report_id = PS3_RUMBLE_ID;
        m_output_report.rumble.padding = 0x01;
        m_output_report.rumble.right_duration = 0xFF;
        m_output_report.rumble.left_duration = 0xFF;
        for (auto &led : m_output_report.led)
        {
            led.time_enabled = 0xFF;
            led.duty_length = 0x27;
            led.enabled = 0x10;
            led.duty_off = 0;
            led.duty_on = 0x32;
        }
    }

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    void update(bool full_poll, bool send_events) override;
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(proto_Output& type);
    uint16_t tick_analog(proto_Output& type);
    bool tick_axis_digital(proto_Output& type) override;
    uint16_t tick_button_pressure(proto_Output& type) override;
    void set_rumble(uint8_t left, uint8_t right) override;
    void set_player_led(uint8_t player) override;
    void set_euphoria_led(bool state) override;
    bool has_rumble() const override { return !m_dancepad && !m_switch_arcade; }
    bool has_player_led() const override { return !m_switch_arcade; }
    bool has_euphoria_led() const override { return m_subtype == DjHeroTurntable; }
    bool has_stagekit_led() const override { return subtype_supports_stagekit(m_subtype); }
    void set_stagekit_led(uint8_t param, uint8_t command) override;

private:
    bool send_ps3_output();
    bool submit_ps3_output(uint8_t report_id, const void *report, uint8_t len);
    bool send_ps3_player_led();
    bool enable_pro_instrument_full_report();
    void complete_stagekit_command();
    bool m_output_dirty = true;
    bool m_player_led_dirty = true;
    ps3_output_report m_output_report = {};
    bool m_out_submit_failed = false;
    bool m_last_output_stagekit = false;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_out_buf[64];
    static constexpr uint8_t stagekit_queue_capacity = 8;
    std::array<std::array<uint8_t, 2>, stagekit_queue_capacity> m_stagekit_commands = {};
    uint8_t m_stagekit_queue_head = 0;
    uint8_t m_stagekit_queue_count = 0;
    bool m_stagekit_queue_overflow_reported = false;
    std::array<uint8_t, 2> m_last_stagekit_command = {};
    bool m_stagekit_set = false;
    bool m_rb2;
    bool m_ion;
    bool m_wt;
    bool m_third_party;
    bool m_dancepad = false;
    bool m_valid_dancepad_report = false;
    bool m_switch_arcade = false;
    bool m_valid_switch_arcade_report = false;
    uint32_t m_last_ghl_poke = 0;
    uint32_t m_last_pro_instrument_poke = 0;
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
