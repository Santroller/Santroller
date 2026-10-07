#pragma once
#include "base.hpp"
#include "device.pb.h"

// Drives the pins of a battery module (eg a TP4056 charger with a boost converter) based on
// how long it has been since the last input. A heartbeat keeps the module from shutting off
// under a light load while the controller is used, and an inactivity output turns it off.
class PowerManagementDevice : public Device
{
public:
    ~PowerManagementDevice() {}
    PowerManagementDevice(proto_PowerManagementDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);

private:
    void update_heartbeat(uint32_t now, uint32_t idle);
    void update_inactivity(uint32_t now, uint32_t idle);
    void set_heartbeat(bool active);
    void set_inactivity(bool active);

    proto_PowerManagementDevice m_device;
    int32_t m_heartbeat_pin;
    int32_t m_inactivity_pin;
    bool m_heartbeat_started = false;
    uint32_t m_last_heartbeat = 0;
    bool m_pulsing = false;
    bool m_pulses_done = false;
    uint32_t m_pulses_start = 0;
};
