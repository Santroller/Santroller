#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/crkd_drum.hpp"
#include <memory>
class CrkdDrumInput : public Input
{
public:
    CrkdDrumInput(proto_CrkdDrumAxisInput input, std::shared_ptr<CrkdDrumDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_CrkdDrumAxisInput m_input;
    std::shared_ptr<CrkdDrumDevice> m_device;
};