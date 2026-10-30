#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/wii.hpp"
#include "profiles/profile.hpp"
#include <memory>
class WiiAxisInput : public Input
{
public:
    WiiAxisInput(proto_WiiAxisInput input, std::shared_ptr<WiiDevice> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    void link_device(bool claim_devices) override
    {
        m_device = m_profile->find_slot_device<WiiDevice>(m_input.deviceid, claim_devices);
    };
    uint32_t linked_device_serial() const override { return m_device ? m_device->m_serial : 0; }

private:
    void setup();
    proto_WiiAxisInput m_input;
    std::shared_ptr<WiiDevice> m_device;
    Profile *m_profile;
};
class WiiButtonInput : public Input
{
public:
    WiiButtonInput(proto_WiiButtonInput input, std::shared_ptr<WiiDevice> device, Profile* profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_WiiButton) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.button); }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    void link_device(bool claim_devices) override
    {
        m_device = m_profile->find_slot_device<WiiDevice>(m_input.deviceid, claim_devices);
    };
    uint32_t linked_device_serial() const override { return m_device ? m_device->m_serial : 0; }

private:
    void setup();
    proto_WiiButtonInput m_input;
    std::shared_ptr<WiiDevice> m_device;
    Profile *m_profile;
};