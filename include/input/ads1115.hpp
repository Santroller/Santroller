#include "input.hpp"
#include "input.pb.h"
#include "devices/ads1115.hpp"
#include <memory>
#pragma once
class ADS1115Input: public Input {
   public:
    ADS1115Input(proto_ADS1115Input input, std::shared_ptr<ADS1115Device> device);
    bool tickDigital();
    uint16_t tickAnalog();
   private:
    void setup();
    uint8_t m_channel;
    proto_ADS1115Input m_input;
    std::shared_ptr<ADS1115Device> m_device;
};