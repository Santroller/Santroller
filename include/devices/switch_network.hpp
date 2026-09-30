#pragma once
#include "base.hpp"
#include "device.pb.h"
#include <stdint.h>

class SwitchNetworkDevice : public Device
{
public:
    ~SwitchNetworkDevice() {}
    SwitchNetworkDevice(proto_SwitchNetworkDevice device, uint16_t id);

    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);

    bool using_pin(uint8_t pin);
    bool read_switch(uint8_t pin, uint8_t other_pin);
    bool read_button(uint8_t button);

private:
    proto_SwitchNetworkDevice m_device;
};
