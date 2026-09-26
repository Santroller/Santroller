#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/crkd_drum.hpp"
#include "profiles/profile.hpp"
#include <memory>
class CrkdDrumInput : public Input
{
public:
    CrkdDrumInput(proto_CrkdDrumAxisInput input, std::shared_ptr<CrkdDrumDevice> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();

private:
    void setup();
    proto_CrkdDrumAxisInput m_input;
    std::shared_ptr<CrkdDrumDevice> m_device;
};