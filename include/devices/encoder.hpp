#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "quadrature_encoder.h"
class EncoderDevice : public Device
{
public:
    ~EncoderDevice() {}
    EncoderDevice(proto_EncoderDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    QuadratureEncoder encoder;

private:
    proto_EncoderDevice m_device;
    uint32_t m_deltaRate;
    uint32_t m_lastPoll;
};