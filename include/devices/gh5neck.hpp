#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "guitar_hero_5_neck.hpp"
class GH5NeckDevice : public Device
{
public:
    ~GH5NeckDevice() {}
    GH5NeckDevice(proto_GuitarHero5NeckDevice device, uint16_t id);
    void update(bool full_poll);
    bool green;
    bool red;
    bool yellow;
    bool blue;
    bool orange;
    bool tapGreen;
    bool tapRed;
    bool tapYellow;
    bool tapBlue;
    bool tapOrange;

private:
    proto_GuitarHero5NeckDevice m_device;
    GuitarHero5Neck m_gh5_neck;
    uint32_t m_lastValue = 0;
};