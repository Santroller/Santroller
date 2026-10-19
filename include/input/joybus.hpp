#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/joybus.hpp"
#include <memory>
#include "profiles/profile.hpp"
class JoybusAxisInput : public Input
{
public:
    JoybusAxisInput(proto_JoybusAxisInput input, std::shared_ptr<JoybusDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_claimed_device(DeviceSlotKind::Joybus, m_input.deviceid, !claim_devices);
        if (!device) {
            auto it = m_profile->devices.find(m_input.deviceid);
            if (it != m_profile->devices.end()) device = it->second;
        }
        m_device = device && device->is_joybus_device() ? std::static_pointer_cast<JoybusDevice>(device) : nullptr;
    };
    uint32_t linked_device_serial() const override { return m_device ? m_device->m_serial : 0; }

private:
    void setup();
    proto_JoybusAxisInput m_input;
    std::shared_ptr<JoybusDevice> m_device;
    Profile *m_profile;
};
class JoybusButtonInput : public Input
{
public:
    JoybusButtonInput(proto_JoybusButtonInput input, std::shared_ptr<JoybusDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_claimed_device(DeviceSlotKind::Joybus, m_input.deviceid, !claim_devices);
        if (!device) {
            auto it = m_profile->devices.find(m_input.deviceid);
            if (it != m_profile->devices.end()) device = it->second;
        }
        m_device = device && device->is_joybus_device() ? std::static_pointer_cast<JoybusDevice>(device) : nullptr;
    };
    uint32_t linked_device_serial() const override { return m_device ? m_device->m_serial : 0; }
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_JoybusButton) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.button); }

private:
    void setup();
    proto_JoybusButtonInput m_input;
    std::shared_ptr<JoybusDevice> m_device;
    Profile *m_profile;
};
