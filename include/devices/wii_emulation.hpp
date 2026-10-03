#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "wii_extension_emulation.hpp"
#include <unordered_map>
#include <memory>
#include <set>

class WiiExtensionEmulationDevice : public Device
{
public:
    ~WiiExtensionEmulationDevice() {}
    WiiExtensionEmulationDevice(const DeviceReloadState *state, proto_WiiEmulationDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    uint16_t read_axis(proto_PS2AxisType type);
    bool read_button(proto_PS2ButtonType type);
    void rescan(bool first);
    bool using_pin(uint8_t pin);
    void save_reload_state(DeviceReloadState &state) const override;
    bool matches_reload_config(const proto_Device &config) const override;
    bool is_communicating() const { return m_controller.is_communicating(); }
    WiiExtensionEmulation &get_controller() { return m_controller; }
    const proto_WiiEmulationDevice &get_config() const { return m_device; }

private:
    proto_WiiEmulationDevice m_device;
    WiiExtensionEmulation m_controller;
    bool m_last_communicating = false;
};