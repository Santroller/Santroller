#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/ps2.hpp"
#include <memory>
class PS2AxisInput : public Input
{
public:
    PS2AxisInput(proto_PS2AxisInput input, std::shared_ptr<PS2Device> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_PS2AxisInput m_input;
    std::shared_ptr<PS2Device> m_device;
};
class PS2ButtonInput : public Input
{
public:
    PS2ButtonInput(proto_PS2ButtonInput input, std::shared_ptr<PS2Device> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_PS2ButtonInput m_input;
    std::shared_ptr<PS2Device> m_device;
};
