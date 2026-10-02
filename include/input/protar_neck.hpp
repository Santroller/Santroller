#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/protar_neck.hpp"
#include "profiles/profile.hpp"
#include <memory>
class ProtarNeckAxisInput : public Input
{
public:
    ProtarNeckAxisInput(proto_ProtarNeckAxisInput input, std::shared_ptr<ProtarNeckDevice> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_ProtarNeckAxisInput m_input;
    std::shared_ptr<ProtarNeckDevice> m_device;
};
class ProtarNeckButtonInput : public Input
{
public:
    ProtarNeckButtonInput(proto_ProtarNeckButtonInput input, std::shared_ptr<ProtarNeckDevice> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    uint64_t hardware_id() const override {
        return (static_cast<uint64_t>(InputHw_ProtarNeck) << 56) |
               (static_cast<uint64_t>(m_device ? m_device->m_id : 0) << 16) |
               static_cast<uint64_t>(m_input.button);
    }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
private:
    void setup();
    proto_ProtarNeckButtonInput m_input;
    std::shared_ptr<ProtarNeckDevice> m_device;
};