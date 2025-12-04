#pragma once
#include "base.hpp"
#include "device.pb.h"
class DebugDevice : public Device
{
public:
    ~DebugDevice() {}
    DebugDevice(proto_DebugDevice device, uint16_t id);
    void update(bool full_poll);
    uart_inst_t *uart_inst;
};