#pragma once
#include "devices/usb/host/host.hpp"
#include "protocols/xbox_one.hpp"

extern "C" {
#include "gip_device.h"
#include "gip_report_queue.h"
}

class XboxOneHost : public UsbHostInterface
{
public:
    XboxOneHost(uint8_t dev_addr, uint8_t interface, uint16_t id);
    ~XboxOneHost();
    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t* out_len);
    bool tick_digital(proto_Output& type);
    uint16_t tick_analog(proto_Output& type);
    void update(bool full_poll, bool send_events);
    void send_report_from_host(XGIPProtocol* report);
    
    gip_device_t m_gip_device;  // Shared GIP device state (public for callbacks)
    gip_report_queue_t* m_report_queue;  // Shared report queue (public for callbacks)
    
private:
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64];
    uint8_t m_last_inputs[64];
};