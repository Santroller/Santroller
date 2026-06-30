#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "DmxOutput.h"
class DMXDevice : public LedDevice
{
public:
    ~DMXDevice() {}
    DMXDevice(proto_DMXDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);

private:
    DmxOutput m_dmx;
    proto_DMXDevice m_device;
};