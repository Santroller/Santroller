#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "band_hero_drum.hpp"
class BandHeroDrumDevice : public Device
{
public:
    ~BandHeroDrumDevice() {}
    BandHeroDrumDevice(proto_BandHeroDrumDevice device, uint16_t id);
    void update(bool full_poll);
    bool using_pin(uint8_t pin);

private:
    BandHeroDrum m_band_hero_drum;
    proto_BandHeroDrumDevice m_device;
    uint32_t m_lastValue = 0;
};