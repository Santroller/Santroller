#include "input.hpp"
#include "input.pb.h"
#include "devices/accelerometer.hpp"
#include "profiles/profile.hpp"
#include <memory>
#pragma once
class AccelerometerInput: public Input {
   public:
    AccelerometerInput(proto_AccelerometerInput input, std::shared_ptr<AccelerometerDevice> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
   private:
    void setup();
    uint8_t m_channel;
    proto_AccelerometerInput m_input;
    std::shared_ptr<AccelerometerDevice> m_device;
};