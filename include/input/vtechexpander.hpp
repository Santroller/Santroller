#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/vtechexpander.hpp"
#include <memory>
class VTechExpanderInput : public Input
{
public:
    VTechExpanderInput(proto_VTechGuitarIOExpanderInput input, std::shared_ptr<VTechGuitarIOExpanderDevice> device);
    bool tick_digital();
    uint16_t tick_analog();
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_VTechExpander) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.button); }

private:
    void setup();
    proto_VTechGuitarIOExpanderInput m_input;
    std::shared_ptr<VTechGuitarIOExpanderDevice> m_device;
};