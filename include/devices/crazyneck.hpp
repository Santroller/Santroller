#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "crazy_guitar_neck.hpp"
class CrazyGuitarNeckDevice : public Device
{
public:
    ~CrazyGuitarNeckDevice() {}
    CrazyGuitarNeckDevice(proto_CrazyGuitarNeckDevice device, uint16_t id);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);

private:
    CrazyGuitarNeck m_crazy_guitar_neck;
    proto_CrazyGuitarNeckDevice m_device;
    uint32_t m_lastValue = 0;
};