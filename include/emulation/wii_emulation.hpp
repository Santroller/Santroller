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

class WiiExtensionEmulationDeviceInstance : public Instance
{
public:
    ~WiiExtensionEmulationDeviceInstance();
    WiiExtensionEmulationDeviceInstance(proto_WiiEmulationDevice device);
    void initialize();
    void process(bool full_poll, bool send_events);
private:
    proto_WiiEmulationDevice m_device;
    WiiExtensionEmulation m_controller;
    uint8_t m_report_size;
    uint8_t m_last_format = 1;
    uint8_t m_buttons_low_idx;
    uint8_t m_buttons_high_idx;
    uint8_t m_initial_report[32];
    uint8_t m_buffer[32];
};