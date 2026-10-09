#pragma once
// Fake emulation/usb/device.hpp: just the parts of an emulated USB interface ConfigLoader touches
#include <stdint.h>
#include "config_fakes.hpp"

class Instance
{
public:
    virtual ~Instance() {}
    virtual void initialize() = 0;
};

class UsbDevice : public Instance
{
public:
    uint8_t interface_id = 0;
    void initialize() override { fake_loader::state.calls.push_back("usb_initialize"); }
    static void reset_ep() { fake_loader::state.calls.push_back("reset_ep"); }
};
