#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "libmpr121.hpp"
class MPR121Device : public Device
{
public:
    ~MPR121Device() {}
    MPR121Device(proto_Mpr121Device device, uint16_t id);
    void update(bool resend_events);

private:
    proto_Mpr121Device m_device;
    MPR121 m_mpr121;
    uint32_t m_lastValue = 0;
};