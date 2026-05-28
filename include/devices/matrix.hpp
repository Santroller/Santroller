#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "libmatrix.hpp"
#include <stdint.h>
class MatrixDevice : public Device
{
public:
    ~MatrixDevice() {}
    MatrixDevice(proto_MatrixDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    bool read_pin(uint8_t pin, uint32_t outPins);

private:
    Matrix m_matrix;
    proto_MatrixDevice m_device;
};