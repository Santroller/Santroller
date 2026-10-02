#pragma once
#include "devices/usb/host/host.hpp"
#include "protocols/pdloader.hpp"

class PDLoaderHost : public UsbHostInterface {
public:
    PDLoaderHost(uint8_t addr, uint8_t interface, uint16_t id)
        : UsbHostInterface(addr, interface, id) { m_subtype = ProjectDiva; }
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> device,
        tusb_desc_interface_t const *desc, uint16_t max_len, uint16_t *out_len);
    bool set_config() override;
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t bytes) override;
    bool tick_digital(proto_Output &output) override;
    uint16_t tick_analog(proto_Output &output) override;
    uint32_t slider_touches() const { return m_report.slider_touches(); }

private:
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint16_t m_ep_in_size = 0;
    CFG_TUSB_MEM_ALIGN uint8_t m_buffer[PDLOADER_PACKET_SIZE] = {};
    PDLoaderInputReport m_report;
};
