#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "libmax1704x.hpp"
class Max1704XDevice : public Device
{
public:
    ~Max1704XDevice() {}
    Max1704XDevice(proto_Max1704xDevice device, uint16_t id);
    void update(bool resend_events);

private:
    proto_Max1704xDevice m_device;
    Max1704X m_max1704x;
    uint32_t m_lastValue = 0;
};