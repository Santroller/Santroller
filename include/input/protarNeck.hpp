#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/protar_neck.hpp"
#include <memory>
class ProtarNeckAxisInput : public Input
{
public:
    ProtarNeckAxisInput(proto_ProtarNeckAxisInput input, std::shared_ptr<ProtarNeckDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_ProtarNeckAxisInput m_input;
    std::shared_ptr<ProtarNeckDevice> m_device;
};
class ProtarNeckButtonInput : public Input
{
public:
    ProtarNeckButtonInput(proto_ProtarNeckButtonInput input, std::shared_ptr<ProtarNeckDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_ProtarNeckButtonInput m_input;
    std::shared_ptr<ProtarNeckDevice> m_device;
};