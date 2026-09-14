#pragma once
#include "input/input.hpp"
#include "input.pb.h"
#include "devices/bt/bt_host.hpp"
#include <memory>

/**
 * BTButtonInput — reads a digital button value from a BluetoothHostInterface.
 * Mirrors USBButtonInput but backed by BT instead of USB.
 */
class BTButtonInput : public Input
{
public:
    BTButtonInput(proto_BTButtonInput input, std::shared_ptr<BluetoothHostInterface> device);
    bool tick_digital();
    uint16_t tick_analog();
    uint64_t hardware_id() const override
    {
        return (static_cast<uint64_t>(InputHw_BTButton) << 56) |
               (static_cast<uint64_t>(m_input.deviceid) << 32) |
               (static_cast<uint64_t>(m_input.button.which_mapping) << 16) |
               static_cast<uint16_t>(m_input.button.mapping.keycode);
    }

private:
    void setup();
    proto_BTButtonInput m_input;
    std::shared_ptr<BluetoothHostInterface> m_device;
};

/**
 * BTAxisInput — reads an analog axis value from a BluetoothHostInterface.
 */
class BTAxisInput : public Input
{
public:
    BTAxisInput(proto_BTAxisInput input, std::shared_ptr<BluetoothHostInterface> device);
    bool tick_digital();
    uint16_t tick_analog();

private:
    void setup();
    proto_BTAxisInput m_input;
    std::shared_ptr<BluetoothHostInterface> m_device;
};

