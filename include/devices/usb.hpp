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
class USBHostHardwareDevice : public Device
{
public:
    ~USBHostHardwareDevice() {}
    USBHostHardwareDevice(proto_UsbHostDevice device, uint16_t id);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);

// private:
    proto_UsbHostDevice m_device;
    // MIDI_NAMESPACE::SimpleMidiInterface midiInterface;
    // uint8_t mReportData[128];
    // Parser &mParser;
};