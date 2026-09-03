#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "psx_controller.hpp"
#include <unordered_map>
#include <memory>
#include <set>

class PS2Device : public Device
{
public:
    ~PS2Device() {}
    PS2Device(const DeviceReloadState* state, proto_PSXDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    uint16_t read_axis(proto_PS2AxisType type);
    bool read_button(proto_PS2ButtonType type);
    bool is_ps2_device(PS2ControllerType type);
    void rescan(bool first);
    bool using_pin(uint8_t pin);
    void save_reload_state(DeviceReloadState& state) const override;

private:
    PSXController m_controller;
    proto_PSXDevice m_device;
    uint32_t m_last_value = 0;
    PS2ControllerType m_lastControllerType = PS2ControllerType::PS2ControllerTypeUnknown;
};