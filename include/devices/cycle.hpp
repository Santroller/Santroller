#pragma once
#include "base.hpp"
#include "device.pb.h"
#include <stdio.h>
#include <vector>
class CycleDevice : public Device
{
public:
    CycleDevice(proto_CycleDevice device, uint16_t id, uint32_t current, std::vector<uint32_t> states);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    void cycle();
    uint32_t get_value() {return m_current_value;}
private:
    proto_CycleDevice m_device;
    std::vector<uint32_t> m_states;
    uint32_t m_current_value;
    uint32_t m_current_index;
};