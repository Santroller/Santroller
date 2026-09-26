#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/crkd.hpp"
#include "profiles/profile.hpp"
#include <memory>
class CrkdButtonInput : public Input
{
public:
    CrkdButtonInput(proto_CrkdNeckButtonInput input, std::shared_ptr<CrkdDevice> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_Crkd) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.button); }

private:
    void setup();
    proto_CrkdNeckButtonInput m_input;
    std::shared_ptr<CrkdDevice> m_device;
};