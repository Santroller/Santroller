#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "dj_hero_turntable.hpp"
class DjHeroTurntableDevice : public Device
{
public:
    ~DjHeroTurntableDevice() {}
    DjHeroTurntableDevice(proto_DJHeroTurntableDevice device, uint16_t id);
    void update();

private:
    proto_DJHeroTurntableDevice m_device;
    DJHeroTurntable m_turntable;
    uint32_t m_lastValue = 0;
};