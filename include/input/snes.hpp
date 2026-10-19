#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/snes.hpp"
#include <memory>
#include "profiles/profile.hpp"
class SNESAxisInput : public Input
{
public:
    SNESAxisInput(proto_SNESAxisInput input, std::shared_ptr<SNESDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_claimed_device(DeviceSlotKind::SNES, m_input.deviceid, !claim_devices);
        if (!device) {
            auto it = m_profile->devices.find(m_input.deviceid);
            if (it != m_profile->devices.end()) device = it->second;
        }
        m_device = device && device->is_snes_device() ? std::static_pointer_cast<SNESDevice>(device) : nullptr;
    };
    uint32_t linked_device_serial() const override { return m_device ? m_device->m_serial : 0; }

private:
    void setup();
    proto_SNESAxisInput m_input;
    std::shared_ptr<SNESDevice> m_device;
    Profile *m_profile;
};
class SNESButtonInput : public Input
{
public:
    SNESButtonInput(proto_SNESButtonInput input, std::shared_ptr<SNESDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_claimed_device(DeviceSlotKind::SNES, m_input.deviceid, !claim_devices);
        if (!device) {
            auto it = m_profile->devices.find(m_input.deviceid);
            if (it != m_profile->devices.end()) device = it->second;
        }
        m_device = device && device->is_snes_device() ? std::static_pointer_cast<SNESDevice>(device) : nullptr;
    };
    uint32_t linked_device_serial() const override { return m_device ? m_device->m_serial : 0; }
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_SNESButton) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.button); }

private:
    void setup();
    proto_SNESButtonInput m_input;
    std::shared_ptr<SNESDevice> m_device;
    Profile *m_profile;
};
