#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "joybus_controller.hpp"

// An N64 or GameCube controller, read over the single wire joybus line
class JoybusDevice : public Device
{
public:
    ~JoybusDevice() {}
    JoybusDevice(proto_JoybusDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    uint16_t read_axis(proto_JoybusAxisType type);
    bool read_button(proto_JoybusButtonType type);
    bool is_joybus_device() const override { return true; }
    bool using_pin(uint8_t pin);
    bool matches_reload_config(const proto_Device &config) const override;
    void set_rumble(uint8_t left, uint8_t right) override;
    bool has_rumble() const override;
    JoybusControllerType controller_type() const;

private:
    proto_JoybusDevice m_device;
    JoybusController m_controller;
    JoybusControllerType m_last_type = JoybusControllerNone;
};
