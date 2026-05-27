#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/vtechexpander.hpp"
#include <memory>
class VTechExpanderInput : public Input
{
public:
    VTechExpanderInput(proto_VTechGuitarIOExpanderInput input, std::shared_ptr<VTechGuitarIOExpanderDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_VTechGuitarIOExpanderInput m_input;
    std::shared_ptr<VTechGuitarIOExpanderDevice> m_device;
};