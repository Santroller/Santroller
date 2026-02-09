#include "input.hpp"
#include "input.pb.h"
#include "devices/mpr121.hpp"
#include <memory>
#pragma once
class MPR121Input: public Input {
   public:
    MPR121Input(proto_MPR121Input input, std::shared_ptr<MPR121Device> device);
    bool tickDigital();
    uint16_t tickAnalog();
   private:
    void setup();
    proto_MPR121Input m_input;
    std::shared_ptr<MPR121Device> m_device;
};