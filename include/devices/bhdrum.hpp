#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "band_hero_drum.hpp"
class BandHeroDrumDevice : public Device
{
public:
    ~BandHeroDrumDevice() {}
    BandHeroDrumDevice(proto_BandHeroDrumDevice device, uint16_t id);
    void update(bool resend_events);

private:
    proto_WiiDevice m_device;
    BandHeroDrum m_band_hero_drum;
    uint32_t m_lastValue = 0;
};