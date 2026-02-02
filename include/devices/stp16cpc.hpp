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

private:
    proto_STP16CPCDevice m_device;
    STP16CPC m_stp15cpc;
};