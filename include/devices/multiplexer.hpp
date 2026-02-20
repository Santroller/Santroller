#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "libmultiplexer.hpp"
class MultiplexerDevice : public Device
{
public:
    ~MultiplexerDevice() {}
    MultiplexerDevice(proto_MultiplexerDevice device, uint16_t id);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    uint16_t read(uint8_t channel);

private:
    proto_MultiplexerDevice m_device;
    Multiplexer m_multiplexer;
};