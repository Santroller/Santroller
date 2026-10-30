#pragma once
#include <stdint.h>
#include <vector>
#include "device.pb.h"

// Whether a gate's pin reading lets its device load. level is the digital level, mv the voltage for
// analog gates
inline bool device_gate_matches(const proto_DeviceGate &gate, bool level, uint32_t mv)
{
    if (gate.analog)
    {
        return mv >= gate.minMv && mv <= gate.maxMv;
    }
    // Active low with a pull up, like GPIO inputs
    return level != (gate.pinMode == PinMode_PullUp);
}

// A gate's state as it was when the config loaded, and how long its pin has disagreed with it
struct DeviceGateState
{
    // Rides out the contacts bouncing as a connector goes in or out
    static constexpr uint32_t SETTLE_MS = 100;

    uint32_t device_id;
    proto_DeviceGate gate;
    bool open;
    bool changing = false;
    uint32_t changed_since = 0;

    // Returns true once the pin has read differently to when the config loaded for SETTLE_MS
    bool update(bool now_open, uint32_t now)
    {
        if (now_open == open)
        {
            changing = false;
            return false;
        }
        if (!changing)
        {
            changing = true;
            changed_since = now;
        }
        return now - changed_since >= SETTLE_MS;
    }
};

// Reads each gated device's pin once per config load, so the devices whose connector isn't plugged in
// aren't built at all, then keeps watching those pins so plugging a connector in or out loads the
// config again
class DeviceGates
{
public:
    static DeviceGates &instance()
    {
        static DeviceGates gates;
        return gates;
    }

    static constexpr uint32_t POLL_MS = 10;

    // Forgets the last load's readings, so the next check of each gate reads its pin again
    void clear()
    {
        m_gates.clear();
        m_reload_due = false;
    }
    // Whether the device should load. Read once per load, so every pass over the config sees the same
    bool check(uint32_t device_id, const proto_DeviceGate &gate);
    // Returns true when a gate has changed since the config loaded, so it needs loading again
    bool poll(uint32_t now);

private:
    DeviceGates() = default;
    static bool read(const proto_DeviceGate &gate);

    std::vector<DeviceGateState> m_gates;
    uint32_t m_last_poll = 0;
    bool m_reload_due = false;
};
