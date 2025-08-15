#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "crkd_neck.hpp"
class CrkdDevice : public Device
{
public:
    ~CrkdDevice() {}
    CrkdDevice(proto_CrkdNeckDevice device, uint16_t id);
    void update(bool resend_events);
    CrkdNeck neck;

private:
    proto_CrkdNeckDevice m_device;
};