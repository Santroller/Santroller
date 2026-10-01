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
    void link_device(bool claim_devices) override
    {
        auto &claimed = claim_devices ? m_profile->claimed_devices : m_profile->temp_claimed_devices;
        auto it = claimed.find(m_input.deviceid);
        if (it != claimed.end() && it->second) {
            m_device = std::static_pointer_cast<UsbHostInterface>(it->second);
            return;
        }
        if (!claimed.empty()) {
            m_device = std::static_pointer_cast<UsbHostInterface>(claimed.begin()->second);
            return;
        }
        auto st_it = m_profile->devices.find(m_input.deviceid);
        if (st_it != m_profile->devices.end()) {
            m_device = std::static_pointer_cast<UsbHostInterface>(st_it->second);
            return;
        }
        m_device = nullptr;
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
    uint64_t hardware_id() const override
    {
        return (static_cast<uint64_t>(InputHw_USBButton) << 56) |
               (static_cast<uint64_t>(m_input.deviceid) << 32) |
               (static_cast<uint64_t>(m_input.button.which_mapping) << 16) |
               static_cast<uint16_t>(m_input.button.mapping.keycode);
    }
    void link_device(bool claim_devices) override
    {
        auto &claimed = claim_devices ? m_profile->claimed_devices : m_profile->temp_claimed_devices;
        auto it = claimed.find(m_input.deviceid);
        if (it != claimed.end() && it->second) {
            m_device = std::static_pointer_cast<UsbHostInterface>(it->second);
            return;
        }
        if (!claimed.empty()) {
            m_device = std::static_pointer_cast<UsbHostInterface>(claimed.begin()->second);
            return;
        }
        auto st_it = m_profile->devices.find(m_input.deviceid);
        if (st_it != m_profile->devices.end()) {
            m_device = std::static_pointer_cast<UsbHostInterface>(st_it->second);
            return;
        }
        m_device = nullptr;
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
        auto &claimed = claim_devices ? m_profile->claimed_devices : m_profile->temp_claimed_devices;
        auto it = claimed.find(m_input.deviceid);
        if (it != claimed.end() && it->second) {
            m_device = std::static_pointer_cast<UsbHostInterface>(it->second);
            return;
        }
        if (!claimed.empty()) {
            m_device = std::static_pointer_cast<UsbHostInterface>(claimed.begin()->second);
            return;
        }
        auto st_it = m_profile->devices.find(m_input.deviceid);
        if (st_it != m_profile->devices.end()) {
            m_device = std::static_pointer_cast<UsbHostInterface>(st_it->second);
            return;
        }
        m_device = nullptr;
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
        auto &claimed = claim_devices ? m_profile->claimed_devices : m_profile->temp_claimed_devices;
        auto it = claimed.find(m_input.deviceid);
        if (it != claimed.end() && it->second) {
            m_device = std::static_pointer_cast<UsbHostInterface>(it->second);
            return;
        }
        if (!claimed.empty()) {
            m_device = std::static_pointer_cast<UsbHostInterface>(claimed.begin()->second);
            return;
        }
        auto st_it = m_profile->devices.find(m_input.deviceid);
        if (st_it != m_profile->devices.end()) {
            m_device = std::static_pointer_cast<UsbHostInterface>(st_it->second);
            return;
        }
        m_device = nullptr;
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
    void link_device(bool claim_devices) override
    {
        auto &claimed = claim_devices ? m_profile->claimed_devices : m_profile->temp_claimed_devices;
        auto it = claimed.find(m_input.deviceid);
        if (it != claimed.end() && it->second) {
            m_device = std::static_pointer_cast<UsbHostInterface>(it->second);
            return;
        }
        if (!claimed.empty()) {
            m_device = std::static_pointer_cast<UsbHostInterface>(claimed.begin()->second);
            return;
        }
        auto st_it = m_profile->devices.find(m_input.deviceid);
        if (st_it != m_profile->devices.end()) {
            m_device = std::static_pointer_cast<UsbHostInterface>(st_it->second);
            return;
        }
        m_device = nullptr;
    };
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_MouseAxisInput m_input;
    std::shared_ptr<UsbHostInterface> m_device;
    Profile *m_profile;
};