#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/ps2.hpp"
#include <memory>
#include "profiles/profile.hpp"
class PS2AxisInput : public Input
{
public:
    PS2AxisInput(proto_PS2AxisInput input, std::shared_ptr<PS2Device> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_claimed_device(DeviceSlotKind::PS2, m_input.deviceid, !claim_devices);
        if (!device) {
            auto it = m_profile->devices.find(m_input.deviceid);
            if (it != m_profile->devices.end()) device = it->second;
        }
        m_device = device && device->is_ps2_controller() ? std::static_pointer_cast<PS2Device>(device) : nullptr;
    };
    uint32_t linked_device_serial() const override { return m_device ? m_device->m_serial : 0; }

private:
    void setup();
    proto_PS2AxisInput m_input;
    std::shared_ptr<PS2Device> m_device;
    Profile *m_profile;
};
class PS2ButtonInput : public Input
{
public:
    PS2ButtonInput(proto_PS2ButtonInput input, std::shared_ptr<PS2Device> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_claimed_device(DeviceSlotKind::PS2, m_input.deviceid, !claim_devices);
        if (!device) {
            auto it = m_profile->devices.find(m_input.deviceid);
            if (it != m_profile->devices.end()) device = it->second;
        }
        m_device = device && device->is_ps2_controller() ? std::static_pointer_cast<PS2Device>(device) : nullptr;
    };
    uint32_t linked_device_serial() const override { return m_device ? m_device->m_serial : 0; }
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_PS2Button) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.button); }

private:
    void setup();
    proto_PS2ButtonInput m_input;
    std::shared_ptr<PS2Device> m_device;
    Profile *m_profile;
};
