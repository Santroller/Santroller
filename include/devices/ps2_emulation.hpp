#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "psx_emulation.hpp"
#include <unordered_map>
#include <memory>
#include <set>

class PSXEmulationDevice : public Device
{
public:
    ~PSXEmulationDevice() {}
    PSXEmulationDevice(std::shared_ptr<PSXEmulationDevice> old, proto_PSXEmulationDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    uint16_t readAxis(proto_PS2AxisType type);
    bool readButton(proto_PS2ButtonType type);
    bool is_ps2_device(PS2ControllerType type);
    void rescan(bool first);
    bool using_pin(uint8_t pin);

private:
    PSXEmulation m_controller;
    proto_PSXEmulationDevice m_device;
    uint32_t m_lastValue = 0;
    PS2ControllerType m_lastControllerType = PS2ControllerType::PS2ControllerTypeUnknown;
};