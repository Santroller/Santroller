#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "wii_extension.hpp"
class APA102Device : public LedDevice
{
public:
    ~APA102Device() {}
    APA102Device(proto_APA102Device device, uint16_t id);
    void update(bool full_poll);

private:
    proto_APA102Device m_device;
};