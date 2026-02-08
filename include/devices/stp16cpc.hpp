#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "libstp16cpc.hpp"
class STP16CPCDevice : public LedDevice
{
public:
    ~STP16CPCDevice() {}
    STP16CPCDevice(proto_STP16CPCDevice device, uint16_t id);
    void update(bool full_poll);
    bool using_pin(uint8_t pin);

private:
    STP16CPC m_stp15cpc;
    proto_STP16CPCDevice m_device;
};