#pragma once
#include "base.hpp"
#include "device.pb.h"
class DebugDevice : public Device
{
public:
    ~DebugDevice() {}
    DebugDevice(proto_DebugDevice device, uint16_t id);
    void update(bool full_poll);
    bool using_pin(uint8_t pin);
    uart_inst_t *uart_inst;
    proto_DebugDevice m_device;
};