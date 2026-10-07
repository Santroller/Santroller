#pragma once
#include "devices/usb/host/hid/hid_host.h"
#include "protocols/santroller_output.hpp"

// A Santroller in its HID mode, the USB counterpart of BleSantrollerHost.
// Santroller 2: subtype comes from the capabilities report (0x10), so it stays
// enumerating until that arrives. Santroller 1: subtype is the high byte of bcdDevice
// and reports use the older per-subtype layouts.
class SantrollerHost : public HidHost
{
public:
    ~SantrollerHost() {}
    SantrollerHost(uint8_t dev_addr, uint8_t interface, uint16_t id, bool is_v2) : HidHost(dev_addr, interface, id), m_is_v2(is_v2)
    {
        // a Santroller 2 isn't assignable until its capabilities report arrives
        m_delayed_init = is_v2;
        m_ready = !is_v2;
    }

    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info);
    bool tick_digital(proto_Output &type);
    uint16_t tick_analog(proto_Output &type);
    uint16_t tick_button_pressure(proto_Output &type);

    void update(bool full_poll, bool send_events) override;
    void set_rumble(uint8_t left, uint8_t right) override { sync_output(); m_output.set_rumble(left, right); }
    void set_player_led(uint8_t player) override { m_output.set_player(player); }
    void set_lightbar(uint8_t r, uint8_t g, uint8_t b) override { m_output.set_rgb(r, g, b); }
    void set_euphoria_led(bool state) override { sync_output(); m_output.set_euphoria(state); }
    void set_stagekit_led(uint8_t param, uint8_t command) override { sync_output(); m_output.set_stagekit(param, command); }
    bool has_rumble() const override { return m_output.has_rumble(); }
    bool has_player_led() const override { return m_output.supports(CapabilityHasStandardPlayerLeds); }
    bool has_lightbar() const override { return m_output.supports(CapabilityHasRGBIndicatorLed); }
    bool has_euphoria_led() const override { return m_output.has_euphoria(); }
    bool has_stagekit_led() const override { return m_output.has_stagekit(); }

private:
    void sync_output() { m_output.subtype = m_subtype; }
    SantrollerOutputState m_output;
    bool m_configured = false;
    void handle_report(const uint8_t *data, uint16_t len);
    void request_capabilities();

    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint8_t m_ep_in_size = 0;
    uint8_t m_ep_out_size = 0;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
    uint8_t m_report[64] = {};
    bool m_is_v2;
    bool m_ready;
    uint8_t m_capabilities = 0;
    uint8_t m_query_attempts = 0;
};
