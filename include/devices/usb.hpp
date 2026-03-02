#pragma once
#include <MIDI.h>

#include <SimpleMidiTransport.hpp>
#include <memory>
#include "i2c.hpp"
#include "base.hpp"
#include "device.pb.h"
#include "libmpr121.hpp"
#include "usb/host/host.hpp"
#include <unordered_map>
extern std::unordered_map<uint8_t, std::shared_ptr<UsbHostDevice>> host_devices;
class USBHostHardwareDevice : public UsbHostInterface
{
public:
    ~USBHostHardwareDevice() {printf("~USBHostHardwareDevice()\r\n");}
    USBHostHardwareDevice(proto_UsbHostDevice device, uint16_t id);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    bool tick_digital(UsbButtonType type) { return false; }
    uint16_t tick_analog(UsbAxisType type) { return 0; }
    bool set_config() { return false; }
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) { return false; }

    // private:
    proto_UsbHostDevice m_device;
    // MIDI_NAMESPACE::SimpleMidiInterface midiInterface;
    // uint8_t mReportData[128];
    // Parser &mParser;
};