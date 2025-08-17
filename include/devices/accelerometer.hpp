#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "accelerometerlib.hpp"
class AccelerometerDevice : public Device
{
public:
    ~AccelerometerDevice() {}
    AccelerometerDevice(proto_AccelerometerDevice device, uint16_t id);
    void update(bool full_poll);

private:
    proto_AccelerometerDevice m_device;
    Accelerometer m_accelerometer;
    uint32_t m_lastValue = 0;
};