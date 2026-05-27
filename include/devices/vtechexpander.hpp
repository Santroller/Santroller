#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "vtech_guitar_ioexpander.hpp"
class VTechGuitarIOExpanderDevice: public Device
{
public:
    ~VTechGuitarIOExpanderDevice() {}
    VTechGuitarIOExpanderDevice(proto_VTechGuitarIOExpanderDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    bool read_button(uint8_t pin);
    void set_led(uint8_t i, uint8_t val);

private:
    VTechGuitarIOExpander m_vtech_expander;
    proto_VTechGuitarIOExpanderDevice m_device;
    uint32_t m_lastValue = 0;
};