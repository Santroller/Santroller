#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "crazy_guitar_neck.hpp"
class CrazyGuitarNeckDevice : public Device
{
public:
    ~CrazyGuitarNeckDevice() {}
    CrazyGuitarNeckDevice(proto_CrazyGuitarNeckDevice device, uint16_t id);
    void update(bool resend_events);

private:
    proto_CrazyGuitarNeckDevice m_device;
    CrazyGuitarNeck m_crazy_guitar_neck;
    uint32_t m_lastValue = 0;
};