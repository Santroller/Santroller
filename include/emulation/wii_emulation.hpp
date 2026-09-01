#pragma once
#include <stdint.h>
#include <memory>
#include "i2c.hpp"
#include "wii_extension_emulation.hpp"
#include "devices/base.hpp"
#include "device.pb.h"

#include "tusb_config.h"
#include "tusb.h"
#include "device.pb.h"
#include "instance.hpp"
#include <vector>
#include <memory>

class WiiEmulationDeviceInstance : public Instance
{
public:
    ~WiiEmulationDeviceInstance();
    WiiEmulationDeviceInstance(proto_WiiEmulationDevice device);
    void initialize();
    void process(bool full_poll, bool send_events);
private:
    proto_WiiEmulationDevice m_device;
    WiiExtensionEmulation m_controller;
    uint8_t m_size;
    uint8_t lastFormat = 1;
    uint8_t buttonsLowIdx;
    uint8_t buttonsHighIdx;
    uint8_t initialReport[32];
    uint8_t m_buffer[32];
};