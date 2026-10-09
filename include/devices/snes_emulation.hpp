#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "libsnes_emulation.hpp"

// Presents as a SNES or NES controller on a console's controller port
class SNESEmulationDevice : public Device
{
public:
    ~SNESEmulationDevice() {}
    SNESEmulationDevice(const DeviceReloadState *state, proto_SNESEmulationDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    void save_reload_state(DeviceReloadState &state) const override;
    bool matches_reload_config(const proto_Device &config) const override;
    bool is_communicating() const { return m_controller.is_communicating(); }
    SnesEmulation &get_controller() { return m_controller; }
    const proto_SNESEmulationDevice &get_config() const { return m_device; }

private:
    proto_SNESEmulationDevice m_device;
    SnesEmulation m_controller;
    bool m_last_communicating = false;
};
