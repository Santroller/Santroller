#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "crkd_drum_device.hpp"
class CrkdDrumDevice : public Device
{
public:
    ~CrkdDrumDevice() {}
    CrkdDrumDevice(proto_CrkdDrumDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    CrkdDrum drum;

private:
    proto_CrkdDrumDevice m_device;
};