#pragma once
#include "host.hpp"
#include "protocols/xinput.hpp"

class XInputGamepadHost : public UsbHostInterface
{
public:
    ~XInputGamepadHost() {}
    XInputGamepadHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
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
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[sizeof(XInputGamepad_Data_t)];
};
class XInputAudioHost : public UsbHostInterface
{
public:
    ~XInputAudioHost() {}
    XInputAudioHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    bool set_config() { return true; }
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) { return true; }
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t* out_len);
    bool tick_digital(UsbButtonType type) { return false; }
    uint16_t tick_analog(UsbAxisType type) { return 0; }
};
class XInputModuleHost : public UsbHostInterface
{
public:
    ~XInputModuleHost() {}
    XInputModuleHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    bool set_config() { return true; }
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) { return true; }
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t* out_len);
    bool tick_digital(UsbButtonType type) { return false; }
    uint16_t tick_analog(UsbAxisType type) { return 0; }
};
class XInputSecurityHost : public UsbHostInterface
{
public:
    ~XInputSecurityHost() {}
    XInputSecurityHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    bool set_config() { return true; }
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) { return true; }
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t* out_len);
    bool tick_digital(UsbButtonType type) { return false; }
    uint16_t tick_analog(UsbAxisType type) { return 0; }
};

class XInputBigButtonHost : public UsbHostInterface
{
public:
    ~XInputBigButtonHost() {}
    XInputBigButtonHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    bool set_config() { return true; }
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) { return true; }
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t* out_len);
    bool tick_digital(UsbButtonType type);
    uint16_t tick_analog(UsbAxisType type);

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
    ~XInputWirelessGamepadHost() {}
    XInputWirelessGamepadHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
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
    uint8_t m_ep_in_buf[64];
    uint8_t m_report_buf[64];
    bool m_found = false;
    uint32_t m_check_caps = 0;
    uint32_t m_check_link = 0;
};
class XInputWirelessAudioHost : public UsbHostInterface
{
public:
    ~XInputWirelessAudioHost() {}
    XInputWirelessAudioHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
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
    uint8_t m_ep_in_buf[64];
};