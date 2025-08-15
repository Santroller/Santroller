#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/crkd.hpp"
#include <memory>
class CrkdButtonInput : public Input
{
public:
    CrkdButtonInput(proto_CrkdNeckButtonInput input, std::shared_ptr<CrkdDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_CrkdNeckButtonInput m_input;
    std::shared_ptr<CrkdDevice> m_device;
};