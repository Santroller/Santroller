#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "crkd_neck.hpp"
class CrkdDevice : public Device
{
public:
    ~CrkdDevice() {}
    CrkdDevice(proto_CrkdNeckDevice device, uint16_t id);
    void begin();
    void end();
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    CrkdNeck neck;

private:
    proto_CrkdNeckDevice m_device;
};