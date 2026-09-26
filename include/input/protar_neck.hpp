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

private:
    void setup();
    proto_ProtarNeckButtonInput m_input;
    std::shared_ptr<ProtarNeckDevice> m_device;
};