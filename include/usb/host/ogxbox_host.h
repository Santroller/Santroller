#pragma once
#include "host.hpp"
#include "protocols/og_xbox.hpp"

class OGXboxHost : public UsbHostInterface
{
public:
    ~OGXboxHost() {}
    OGXboxHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    bool set_config();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t* out_len);
    bool tick_digital(UsbButtonType type);
    uint16_t tick_analog(UsbAxisType type);

private:
    uint8_t m_ep_in;
    uint8_t m_ep_out;
    uint8_t m_ep_in_size;
    uint8_t m_ep_out_size;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[sizeof(OGXboxGamepad_Data_t)];
};