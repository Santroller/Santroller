#pragma once
#include <stdint.h>
#include <memory>
#include "i2c.hpp"
#include "devices/base.hpp"
#include "device.pb.h"
#include "psx_emulation.hpp"

#include "tusb_config.h"
#include "tusb.h"
#include "device.pb.h"
#include "instance.hpp"
#include <vector>
#include <memory>

class Ps2EmulationDeviceInstance : public Instance
{
public:
    ~Ps2EmulationDeviceInstance();
    Ps2EmulationDeviceInstance(proto_PSXEmulationDevice device);
    void initialize();
    void process();
private:
    proto_PSXEmulationDevice m_device;
    PSXEmulation m_controller;
    uint8_t m_size;
    uint8_t initialReport[32];
    uint8_t m_buffer[32];
    uint8_t m_buffer_formatted[32];
};