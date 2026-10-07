#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/crazyneck.hpp"
#include "profiles/profile.hpp"
#include <memory>
class CrazyGuitarNeckButtonInput : public Input
{
public:
    CrazyGuitarNeckButtonInput(proto_CrazyGuitarNeckButtonInput input, std::shared_ptr<CrazyGuitarNeckDevice> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_CrazyGuitarNeck) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.button); }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_CrazyGuitarNeckButtonInput m_input;
    std::shared_ptr<CrazyGuitarNeckDevice> m_device;
};
