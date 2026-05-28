#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/matrix.hpp"
#include <memory>
class MatrixInput : public Input
{
public:
    MatrixInput(proto_MatrixInput input, std::shared_ptr<MatrixDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_MatrixInput m_input;
    std::shared_ptr<MatrixDevice> m_device;
};