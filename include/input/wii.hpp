#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/wii.hpp"
#include <memory>
class WiiAxisInput : public Input
{
public:
    WiiAxisInput(proto_WiiAxisInput input, std::shared_ptr<WiiDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_WiiAxisInput m_input;
    std::shared_ptr<WiiDevice> m_device;
};
class WiiButtonInput : public Input
{
public:
    WiiButtonInput(proto_WiiButtonInput input, std::shared_ptr<WiiDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_WiiButtonInput m_input;
    std::shared_ptr<WiiDevice> m_device;
};