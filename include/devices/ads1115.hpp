#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "libads1115.hpp"
class ADS1115Device : public Device
{
public:
    ~ADS1115Device() {}
    ADS1115Device(proto_ADS1115Device device, uint16_t id);
    void update(bool full_poll);
    ADS1115 ads1115;

private:
    proto_ADS1115Device m_device;
};