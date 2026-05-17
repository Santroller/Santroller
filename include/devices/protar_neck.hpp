#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "protar_neck_device.hpp"
#include <unordered_map>
#include <memory>
#include <set>

class ProtarNeckDevice : public Device
{
public:
    ~ProtarNeckDevice() {}
    ProtarNeckDevice(proto_ProtarNeckDevice device, uint16_t id);
    void update(bool full_poll, bool send_events);
    uint16_t readAxis(ProGuitarAxisType type);
    bool readButton(ProGuitarButtonType type);
    bool using_pin(uint8_t pin);

private:
    ProtarNeck m_controller;
    proto_ProtarNeckDevice m_device;
    uint32_t m_lastValue = 0;
};