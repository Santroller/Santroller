#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/djh.hpp"
#include "profiles/profile.hpp"
#include <memory>
class DJHeroPlatterInput : public Input
{
public:
    DJHeroPlatterInput(proto_DJHeroPlatterInput input, std::shared_ptr<DjHeroTurntableDevice> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_DJHeroPlatter) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.type); }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_DJHeroPlatterInput m_input;
    std::shared_ptr<DjHeroTurntableDevice> m_device;
};
