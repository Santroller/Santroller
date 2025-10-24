#include "input.hpp"
#include "input.pb.h"
#include "devices/accelerometer.hpp"
#include <memory>
#pragma once
class AccelerometerInput: public Input {
   public:
    AccelerometerInput(proto_AccelerometerInput input, std::shared_ptr<AccelerometerDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();
   private:
    void setup();
    uint8_t m_channel;
    proto_AccelerometerInput m_input;
    std::shared_ptr<AccelerometerDevice> m_device;
};