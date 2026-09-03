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
    PSXEmulationDevice(proto_PSXEmulationDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    uint16_t read_axis(proto_PS2AxisType type);
    bool read_button(proto_PS2ButtonType type);
    void rescan(bool first);
    bool using_pin(uint8_t pin);

private:
    proto_PSXEmulationDevice m_device;
};