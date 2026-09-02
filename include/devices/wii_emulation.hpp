#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include <unordered_map>
#include <memory>
#include <set>

class WiiExtensionEmulationDevice : public Device
{
public:
    ~WiiExtensionEmulationDevice() {}
    WiiExtensionEmulationDevice(proto_WiiEmulationDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    uint16_t readAxis(proto_PS2AxisType type);
    bool readButton(proto_PS2ButtonType type);
    void rescan(bool first);
    bool using_pin(uint8_t pin);

private:
    proto_WiiEmulationDevice m_device;
};