#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "SNESpad.h"

// A SNES pad, NES pad or SNES mouse on the latch / clock / data shift register bus
class SNESDevice : public Device
{
public:
    ~SNESDevice() {}
    SNESDevice(proto_SNESDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    uint16_t read_axis(proto_SNESAxisType type);
    bool read_button(proto_SNESButtonType type);
    bool is_snes_device() const override { return true; }
    bool using_pin(uint8_t pin);
    bool matches_reload_config(const proto_Device &config) const override;
    SNESControllerType controller_type() const;

private:
    proto_SNESDevice m_device;
    SNESpad m_pad;
    uint32_t m_last_poll_us = 0;
    SNESControllerType m_last_type = SNESControllerNone;
};
