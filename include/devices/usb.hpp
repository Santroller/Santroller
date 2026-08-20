#pragma once
#include <memory>
#include "i2c.hpp"
#include "base.hpp"
#include "device.pb.h"
#include "libmpr121.hpp"
#include "usb/host/host.hpp"
extern std::array<std::shared_ptr<UsbHostDevice>,127> host_devices;
class USBHostHardwareDevice : public UsbHostInterface
{
public:
    ~USBHostHardwareDevice();
    USBHostHardwareDevice(proto_UsbHostDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    void rescan(bool first);
    bool using_pin(uint8_t pin);
    bool tick_digital(proto_Output& type) { return false; }
    uint16_t tick_analog(proto_Output& type) { return 0; }
    bool set_config() { return false; }
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) { return false; }

private:
    proto_UsbHostDevice m_device;
};