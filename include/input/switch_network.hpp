#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/switch_network.hpp"
#include "profiles/profile.hpp"
#include <memory>

class SwitchNetworkInput : public Input
{
public:
    SwitchNetworkInput(proto_SwitchNetworkInput input, std::shared_ptr<SwitchNetworkDevice> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    uint64_t hardware_id() const override {
        return (static_cast<uint64_t>(InputHw_SwitchNetwork) << 56) |
               (static_cast<uint64_t>(m_input.deviceid) << 32) |
               static_cast<uint32_t>(m_input.button);
    }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_SwitchNetworkInput m_input;
    std::shared_ptr<SwitchNetworkDevice> m_device;
};
