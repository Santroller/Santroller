#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/matrix.hpp"
#include <memory>
class MatrixInput : public Input
{
public:
    MatrixInput(proto_MatrixInput input, std::shared_ptr<MatrixDevice> device);
    bool tick_digital();
    uint16_t tick_analog();
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_Matrix) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 32) | (static_cast<uint64_t>(m_input.pin) << 16) | static_cast<uint32_t>(m_input.outputPin); }

private:
    void setup();
    proto_MatrixInput m_input;
    std::shared_ptr<MatrixDevice> m_device;
};