#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "libjoybus_emulation.hpp"

// Presents as a GameCube or N64 controller on a console's controller port
class JoybusEmulationDevice : public Device
{
public:
    ~JoybusEmulationDevice() {}
    JoybusEmulationDevice(const DeviceReloadState *state, proto_JoybusEmulationDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    void save_reload_state(DeviceReloadState &state) const override;
    bool matches_reload_config(const proto_Device &config) const override;
    bool is_communicating() const { return m_controller.is_communicating(); }
    JoybusEmulation &get_controller() { return m_controller; }
    const proto_JoybusEmulationDevice &get_config() const { return m_device; }

private:
    proto_JoybusEmulationDevice m_device;
    JoybusEmulation m_controller;
    bool m_last_communicating = false;
};
