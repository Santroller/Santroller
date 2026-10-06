#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/usb.hpp"
#include "devices/usb/host/host.hpp"
#include "profiles/profile.hpp"
#include <memory>
class USBAxisInput : public Input
{
public:
    USBAxisInput(proto_USBAxisInput input, std::shared_ptr<UsbHostInterface> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_claimed_device(DeviceSlotKind::USB, m_input.deviceid, !claim_devices);
        if (!device) {
            auto it = m_profile->devices.find(m_input.deviceid);
            if (it != m_profile->devices.end()) device = it->second;
        }
        m_device = device && device->is_usb_host_interface() ? std::static_pointer_cast<UsbHostInterface>(device) : nullptr;
    };
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_USBAxisInput m_input;
    std::shared_ptr<UsbHostInterface> m_device;
    Profile *m_profile;
};
class USBButtonInput : public Input
{
public:
    USBButtonInput(proto_USBButtonInput input, std::shared_ptr<UsbHostInterface> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool tick_pro_key_range(uint32_t &active_keys, uint8_t *velocities, uint8_t key_count) override;
    bool has_independent_analog_value() const override { return true; }
    uint64_t hardware_id() const override
    {
        return (static_cast<uint64_t>(InputHw_USBButton) << 56) |
               (static_cast<uint64_t>(m_input.deviceid) << 32) |
               (static_cast<uint64_t>(m_input.button.which_mapping) << 16) |
               static_cast<uint16_t>(m_input.button.mapping.keycode);
    }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_claimed_device(DeviceSlotKind::USB, m_input.deviceid, !claim_devices);
        if (!device) {
            auto it = m_profile->devices.find(m_input.deviceid);
            if (it != m_profile->devices.end()) device = it->second;
        }
        m_device = device && device->is_usb_host_interface() ? std::static_pointer_cast<UsbHostInterface>(device) : nullptr;
    };
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_USBButtonInput m_input;
    std::shared_ptr<UsbHostInterface> m_device;
    Profile *m_profile;
};
class KeyboardKeyInput : public Input
{
public:
    KeyboardKeyInput(proto_KeyboardKeyInput input, std::shared_ptr<UsbHostInterface> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_KeyboardKey) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.key); }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_claimed_device(DeviceSlotKind::USB, m_input.deviceid, !claim_devices);
        if (!device) {
            auto it = m_profile->devices.find(m_input.deviceid);
            if (it != m_profile->devices.end()) device = it->second;
        }
        m_device = device && device->is_usb_host_interface() ? std::static_pointer_cast<UsbHostInterface>(device) : nullptr;
    };
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_KeyboardKeyInput m_input;
    std::shared_ptr<UsbHostInterface> m_device;
    Profile *m_profile;
};
class MouseButtonInput : public Input
{
public:
    MouseButtonInput(proto_MouseButtonInput input, std::shared_ptr<UsbHostInterface> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_claimed_device(DeviceSlotKind::USB, m_input.deviceid, !claim_devices);
        if (!device) {
            auto it = m_profile->devices.find(m_input.deviceid);
            if (it != m_profile->devices.end()) device = it->second;
        }
        m_device = device && device->is_usb_host_interface() ? std::static_pointer_cast<UsbHostInterface>(device) : nullptr;
    };
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_MouseButtonInput m_input;
    std::shared_ptr<UsbHostInterface> m_device;
    Profile *m_profile;
};
class MouseAxisInput : public Input
{
public:
    MouseAxisInput(proto_MouseAxisInput input, std::shared_ptr<UsbHostInterface> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return true; }
    void link_device(bool claim_devices) override
    {
        auto device = m_profile->get_claimed_device(DeviceSlotKind::USB, m_input.deviceid, !claim_devices);
        if (!device) {
            auto it = m_profile->devices.find(m_input.deviceid);
            if (it != m_profile->devices.end()) device = it->second;
        }
        m_device = device && device->is_usb_host_interface() ? std::static_pointer_cast<UsbHostInterface>(device) : nullptr;
    };
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_MouseAxisInput m_input;
    std::shared_ptr<UsbHostInterface> m_device;
    Profile *m_profile;
};