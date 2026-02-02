#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "psx_controller.hpp"
class PS2Device : public Device
{
public:
    ~PS2Device() {}
    PS2Device(proto_PSXDevice device, uint16_t id, MultitapPort port);
    void update(bool full_poll);
    void load_devices();
    uint16_t readAxis(proto_PS2AxisType type);
    bool readButton(proto_PS2ButtonType type);
    bool isExtension(PS2ControllerType type);

private:
    proto_PSXDevice m_device;
    PSXController m_controller;
    uint32_t m_lastValue = 0;
    PS2ControllerType m_lastControllerType = PS2ControllerType::PS2ControllerTypeUnknown;
};