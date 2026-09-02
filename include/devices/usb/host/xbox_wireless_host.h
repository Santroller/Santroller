#pragma once
#include "devices/usb/host/host.hpp"
#include "devices/usb/host/xbox/xbox_adapter.h"
#include <memory>
#include <array>

extern "C" {
#include "gip_report_queue.h"
}

extern "C" {
#include "devices/usb/host/xbox/mt76.h"
}

class XboxWirelessController;

class XboxWirelessHost : public UsbHostInterface
{
public:
    ~XboxWirelessHost();
    XboxWirelessHost(uint8_t dev_addr, uint8_t interface, uint16_t id);
    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t* out_len);
    bool tick_digital(proto_Output& type);
    uint16_t tick_analog(proto_Output& type);
    void update(bool full_poll, bool send_events);
    
    void create_controller_interface(uint8_t controller_idx, uint8_t subtype);
    
    void remove_controller_interface(uint8_t controller_idx);
    
    std::shared_ptr<XboxWirelessController> get_controller_interface(uint8_t controller_idx);
    
    int send_gip_to_controller(uint8_t wcid, const uint8_t *mac_addr, const uint8_t *data, uint16_t len);
    
    void send_report_from_host(uint8_t* packet, uint16_t len);
private:
    void initialize_adapter();
    
    CFG_TUSB_MEM_ALIGN uint8_t m_cmd_buf[0x0654];
    // RX bulk aggregation is disabled, so one message per transfer bounds this at a single MPDU.
    CFG_TUSB_MEM_ALIGN uint8_t m_data_buf[0x1000];
    
    struct mt76_dev m_mt76_dev;
    
    std::array<std::shared_ptr<XboxWirelessController>, XBOX_MAX_CONTROLLERS> m_controller_interfaces;
    
    bool m_adapter_initialized;
    bool m_firmware_loaded;
    bool m_radio_initialized;
    bool m_pairing_initialized;
    
    uint32_t m_last_update_time;
    uint32_t m_init_start_time;
    
    gip_report_queue_t* m_report_queue;
};
