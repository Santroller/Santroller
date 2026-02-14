#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "libmax1704x.hpp"
class Max1704XDevice : public Device
{
public:
    ~Max1704XDevice() {}
    Max1704XDevice(proto_Max1704xDevice device, uint16_t id);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);

private:
    Max1704X m_max1704x;
    proto_Max1704xDevice m_device;
    uint32_t m_lastValue = 0;
};