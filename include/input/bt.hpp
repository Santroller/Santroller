#pragma once
#include "protocols/motion.hpp"
#include "input/input.hpp"
#include "input.pb.h"
#include "devices/bt/bt_host.hpp"
#include "profiles/profile.hpp"
#include <memory>

/**
 * BTButtonInput — reads a digital button value from a BluetoothHostInterface.
 * Mirrors USBButtonInput but backed by BT instead of USB.
 */
class BTButtonInput : public Input
{
public:
    BTButtonInput(proto_BTButtonInput input, std::shared_ptr<BluetoothHostInterface> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    uint32_t linked_device_serial() const override { return m_device ? m_device->m_serial : 0; }
    uint64_t hardware_id() const override
    {
        return (static_cast<uint64_t>(InputHw_BTButton) << 56) |
               (static_cast<uint64_t>(m_input.deviceid) << 32) |
               (static_cast<uint64_t>(m_input.button.which_mapping) << 16) |
               static_cast<uint16_t>(m_input.button.mapping.keycode);
    }
    void link_device(bool claim_devices) override
    {
        m_device = m_profile->find_slot_device<BluetoothHostInterface>(m_input.deviceid, claim_devices);
    };

private:
    void setup();
    bool by_position() const;
    proto_BTButtonInput m_input;
    std::shared_ptr<BluetoothHostInterface> m_device;
    Profile *m_profile;
};

/**
 * BTAxisInput — reads an analog axis value from a BluetoothHostInterface.
 */
class BTAxisInput : public Input
{
public:
    BTAxisInput(proto_BTAxisInput input, std::shared_ptr<BluetoothHostInterface> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    bool valid() const override
    {
        // hosts without motion leave Gamepad_Accel / Gamepad_Gyro at the output's resting pose
        bool motion = m_input.axis.which_mapping == proto_Output_gamepadAxis_tag && is_motion_axis(m_input.axis.mapping.gamepadAxis);
        return m_device != nullptr && m_device->valid() && (!motion || m_device->has_motion());
    }
    uint32_t linked_device_serial() const override { return m_device ? m_device->m_serial : 0; }
    void link_device(bool claim_devices) override
    {
        m_device = m_profile->find_slot_device<BluetoothHostInterface>(m_input.deviceid, claim_devices);
    };

private:
    void setup();
    proto_BTAxisInput m_input;
    std::shared_ptr<BluetoothHostInterface> m_device;
    Profile *m_profile;
};
