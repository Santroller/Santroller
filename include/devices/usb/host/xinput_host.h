#pragma once
#include "devices/usb/host/host.hpp"
#include "protocols/xinput.hpp"
#include <atomic>

class XInputGamepadHost : public UsbHostInterface
{
public:
    ~XInputGamepadHost() {}
    XInputGamepadHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    void update(bool full_poll, bool send_events) override;
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len);
    bool tick_digital(proto_Output &type);
    uint16_t tick_analog(proto_Output &type);
    uint16_t tick_button_pressure(proto_Output &type) override;
    void set_rumble(uint8_t left, uint8_t right) override;
    void set_player_led(uint8_t player) override;
    void set_euphoria_led(bool state) override;
    bool has_rumble() const override { return true; }
    bool has_player_led() const override { return true; }
    bool has_euphoria_led() const override { return m_subtype == DjHeroTurntable; }
    bool has_stagekit_led() const override { return subtype_supports_stagekit(m_subtype); }
    void set_stagekit_led(uint8_t param, uint8_t command) override;

private:
    bool send_rumble_report();
    bool send_player_led_report();
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[sizeof(XInputGamepad_Data_t)];
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_out_buf[32];
    uint8_t m_rumble_left = 0;
    uint8_t m_rumble_right = 0;
    uint8_t m_player_led = 0x02;
    bool m_rumble_dirty = false;
    bool m_player_led_dirty = true;
    bool m_last_output_was_led = false;
    bool m_pending_player_led = false;
    bool m_out_pending = false;
    bool m_out_submit_failed = false;
    std::atomic<bool> m_out_done{false};
    xfer_result_t m_out_result = XFER_RESULT_SUCCESS;
    bool m_euphoria = false;
    bool m_wt = false;
};
class XInputAudioHost : public UsbHostInterface
{
public:
    ~XInputAudioHost() {}
    XInputAudioHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    bool set_config() { return true; }
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) { return true; }
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len);
    bool tick_digital(proto_Output &type) { return false; }
    uint16_t tick_analog(proto_Output &type) { return 0; }
};
class XInputModuleHost : public UsbHostInterface
{
public:
    ~XInputModuleHost() {}
    XInputModuleHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    bool set_config() { return true; }
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) { return true; }
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len);
    bool tick_digital(proto_Output &type) { return false; }
    uint16_t tick_analog(proto_Output &type) { return 0; }
};
class XInputSecurityHost : public UsbHostInterface
{
public:
    ~XInputSecurityHost() {}
    XInputSecurityHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    bool set_config() { return true; }
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) { return true; }
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len);
    bool tick_digital(proto_Output &type) { return false; }
    uint16_t tick_analog(proto_Output &type) { return 0; }
};

class XInputBigButtonHost : public UsbHostInterface
{
public:
    ~XInputBigButtonHost() {}
    XInputBigButtonHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) { return true; }
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len);
    bool tick_digital(proto_Output &type);
    uint16_t tick_analog(proto_Output &type);

private:
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    uint8_t m_ep_in_buf[sizeof(XInputBigButton_Data_t)];
};
class XInputWirelessGamepadHost : public UsbHostInterface
{
public:
    ~XInputWirelessGamepadHost() { set_linked(false); }
    XInputWirelessGamepadHost(uint8_t dev_addr, uint8_t interface, uint16_t id);
    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    void update(bool full_poll, bool send_events) override;
    void disconnect() override;
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len);
    bool tick_digital(proto_Output &type);
    uint16_t tick_analog(proto_Output &type);
    uint16_t tick_button_pressure(proto_Output &type) override;
    void set_rumble(uint8_t left, uint8_t right) override;
    void set_player_led(uint8_t player) override;
    bool has_rumble() const override { return true; }
    bool has_player_led() const override { return true; }

private:
    bool send_out(const char *reason, const uint8_t *packet, uint8_t len);
    void flush_out_queue();
    void process_events();
    void link_restored(const char *why);
    // tracks how many slots have a controller linked, see xinput_wireless_status.hpp
    void set_linked(bool linked);
    bool m_linked = false;
    void disconnect_controller();
    void process_in(const uint8_t *buf, uint32_t len);
    void process_out(xfer_result_t result);
    // IN transfers captured in xfer_cb (producer) and serviced from update (consumer)
    struct InEvent
    {
        xfer_result_t result;
        uint8_t length;
        uint8_t data[64];
    };
    static constexpr uint8_t event_queue_capacity = 8;
    InEvent m_events[event_queue_capacity] = {};
    std::atomic<uint8_t> m_event_head{0};
    std::atomic<uint8_t> m_event_tail{0};
    std::atomic<uint32_t> m_events_dropped{0};
    uint32_t m_events_dropped_reported = 0;
    // Only one OUT transfer is in flight at a time, so a single slot is enough
    std::atomic<bool> m_out_done{false};
    xfer_result_t m_out_result = XFER_RESULT_SUCCESS;
    struct OutCommand
    {
        const char *reason;
        uint8_t length;
        uint8_t data[12];
    };
    static constexpr size_t out_queue_capacity = 6;
    OutCommand m_out_queue[out_queue_capacity] = {};
    size_t m_out_queue_count = 0;
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_out_buf[64];
    uint8_t m_report_buf[64];
    bool m_found = false;
    bool m_request_caps_on_input = false;
    uint8_t m_last_player_led = 0;
    // Set while the data link is down but the slot is still held
    bool m_link_lost = false;
    uint32_t m_link_lost_ms = 0;
    // for the drop trace: when the link last dropped, and last came back up
    uint32_t m_last_drop_ms = 0;
    uint32_t m_link_up_ms = 0;
    static constexpr uint32_t link_loss_grace_ms = 3000;
    bool m_led_set = false;
    bool m_wt = false;
    bool m_out_pending = false;
    uint32_t m_out_submit_failures = 0;
    uint32_t m_input_count = 0;
    uint32_t m_last_input_ms = 0;
    uint32_t m_next_input_stats_ms = 0;
    uint32_t m_reported_input_count = 0;
    uint32_t m_empty_count = 0;
    // Written only by xfer_cb; update() reads and resets (a lost max is harmless)
    volatile uint32_t m_last_in_us = 0;
    volatile uint32_t m_max_in_gap_us = 0;
    uint32_t m_last_update_us = 0;
    uint32_t m_max_update_gap_us = 0;
};
class XInputWirelessAudioHost : public UsbHostInterface
{
public:
    ~XInputWirelessAudioHost() {}
    XInputWirelessAudioHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len);
    bool tick_digital(proto_Output &type);
    uint16_t tick_analog(proto_Output &type);

private:
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
};