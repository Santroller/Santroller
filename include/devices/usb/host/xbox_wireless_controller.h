#pragma once
#include "devices/usb/host/host.hpp"
#include "devices/usb/host/xbox/xbox_adapter.h"

class XboxWirelessHost;

class XboxWirelessController : public UsbHostInterface
{
public:
    XboxWirelessController(XboxWirelessHost* adapter, uint8_t controller_idx, uint8_t dev_addr, uint16_t id);
    ~XboxWirelessController();
    
    bool tick_digital(proto_Output& type) override;
    uint16_t tick_analog(proto_Output& type) override;
    void update(bool full_poll, bool send_events) override;
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) override;
    bool set_config() override;
    void disconnect() override;
    
    uint8_t get_controller_index() const { return m_controller_idx; }
    
    xbox_controller_t* get_controller_data() { return &m_controller; }
    
    void process_gip_data(const uint8_t *data, uint16_t len);
    void send_report_from_host(XGIPProtocol *report);
    
    int send_gip_packet(const uint8_t *data, uint16_t len);
    
    void on_device_descriptor(SubType subtype);
    void on_auth(const uint8_t *data, uint16_t len);
    
    xbox_controller_t m_controller;
    uint8_t m_controller_idx;
    
private:
    XboxWirelessHost* m_adapter;
    bool m_auth_registered = false;
};
