#pragma once
#include "base.hpp"
#include "device.pb.h"
#include <stdio.h>
#include <vector>
#include <memory>
class CycleDevice : public Device
{
public:
    CycleDevice(const DeviceReloadState* state, proto_CycleDevice device, uint16_t id, uint32_t current, std::vector<uint32_t> states);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    void cycle(bool forward);
    void handle_command(proto_Command command) override;
    uint32_t get_value() {return m_current_value;}
    // Whether the forward / backward input was down when last seen. Kept here, as stepping can switch
    // profiles and rebuild the inputs while the button is still held, and the rebuilt input mustn't
    // take that for a new press
    bool input_held(bool forward) const { return forward ? m_forward_held : m_backward_held; }
    void set_input_held(bool forward, bool held) { (forward ? m_forward_held : m_backward_held) = held; }
    void save_reload_state(DeviceReloadState& state) const override;
private:
    uint32_t step_index(bool forward) const;
    void set_index(uint32_t index);
    proto_CycleDevice m_device;
    std::vector<uint32_t> m_states;
    uint32_t m_current_value;
    uint32_t m_current_index;
    uint32_t m_last = 0;
    bool m_forward_held = false;
    bool m_backward_held = false;
};