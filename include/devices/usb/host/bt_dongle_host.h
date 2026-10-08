#pragma once
#include "devices/usb/host/host.hpp"
#include "devices/bt/usb_dongle.hpp"

// A USB bluetooth adapter plugged into the host port, used as the bluetooth controller on boards
// without one (e.g. a Pico instead of a Pico W). This only moves HCI packets between the adapter
// and BTstack, see usb_dongle.hpp.
class BtDongleHost : public UsbHostInterface
{
public:
    ~BtDongleHost();
    BtDongleHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len);
    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    void disconnect();
    // not something to map inputs from
    bool is_assignable() const override { return false; }
    bool tick_digital(proto_Output &type) { return false; }
    uint16_t tick_analog(proto_Output &type) { return 0; }
    void update(bool full_poll, bool send_events) {}

    bool can_send(uint8_t packet_type);
    bool send(uint8_t packet_type, const uint8_t *packet, uint16_t len);

private:
    void receive_event();
    void receive_acl();
    static void command_complete(tuh_xfer_t *xfer);

    uint8_t m_ep_event = 0;
    uint8_t m_ep_acl_in = 0;
    uint8_t m_ep_acl_out = 0;
    uint16_t m_event_packet_size = 0;
    uint16_t m_acl_in_packet_size = 0;
    bool m_command_busy = false;
    bool m_acl_out_busy = false;
    // incoming packets are read a USB packet at a time, and put back together using their HCI headers
    uint16_t m_event_len = 0;
    uint16_t m_acl_len = 0;
    CFG_TUSB_MEM_ALIGN uint8_t m_event_buf[USB_DONGLE_EVENT_BUFFER_SIZE];
    CFG_TUSB_MEM_ALIGN uint8_t m_acl_buf[USB_DONGLE_ACL_BUFFER_SIZE];
    CFG_TUSB_MEM_ALIGN uint8_t m_command_buf[USB_DONGLE_EVENT_BUFFER_SIZE];
    CFG_TUSB_MEM_ALIGN tusb_control_request_t m_command_request;
};
