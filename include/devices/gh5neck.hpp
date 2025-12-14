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
    GuitarHero5Neck m_gh5_neck;

private:
    proto_GuitarHero5NeckDevice m_device;
    uint32_t m_lastValue = 0;
};