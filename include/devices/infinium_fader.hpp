#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "infinium_fader_device.hpp"

class InfiniumFaderDevice : public Device
{
public:
    ~InfiniumFaderDevice() {}
    InfiniumFaderDevice(proto_InfiniumFaderDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    InfiniumFader fader;

private:
    proto_InfiniumFaderDevice m_device;
};
