#pragma once
#include "base.hpp"
#include "device.pb.h"
#include <stdio.h>
#include <vector>
class ToggleDevice : public Device
{
public:
    ToggleDevice(proto_ToggleDevice device, uint16_t id, bool current);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    void toggle();
    bool get_value() {return m_current_value;}
private:
    proto_ToggleDevice m_device;
    bool m_current_value;
    uint32_t m_last = 0;
};