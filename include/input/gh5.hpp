#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/gh5neck.hpp"
#include <memory>
class Gh5ButtonInput : public Input
{
public:
    Gh5ButtonInput(proto_Gh5NeckButtonInput input, std::shared_ptr<GH5NeckDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_Gh5NeckButtonInput m_input;
    std::shared_ptr<GH5NeckDevice> m_device;
};