#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "dj_hero_turntable.hpp"
class DjHeroTurntableDevice : public Device
{
public:
    ~DjHeroTurntableDevice() {}
    DjHeroTurntableDevice(proto_DJHeroTurntableDevice device, uint16_t id);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);

private:
    DJHeroTurntable m_turntable;
    proto_DJHeroTurntableDevice m_device;
    uint32_t m_lastValue = 0;
};