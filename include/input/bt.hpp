#pragma once
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
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    uint64_t hardware_id() const override
    {
        return (static_cast<uint64_t>(InputHw_BTButton) << 56) |
               (static_cast<uint64_t>(m_input.deviceid) << 32) |
               (static_cast<uint64_t>(m_input.button.which_mapping) << 16) |
               static_cast<uint16_t>(m_input.button.mapping.keycode);
    }
    void link_device(bool claim_devices) override
    {
        auto &claimed = claim_devices ? m_profile->claimed_devices : m_profile->temp_claimed_devices;
        auto it = claimed.find(m_input.deviceid);
        if (it != claimed.end() && it->second) {
            m_device = std::static_pointer_cast<BluetoothHostInterface>(it->second);
            return;
        }
        if (!claimed.empty()) {
            m_device = std::static_pointer_cast<BluetoothHostInterface>(claimed.begin()->second);
            return;
        }
        auto st_it = m_profile->devices.find(m_input.deviceid);
        if (st_it != m_profile->devices.end()) {
            m_device = std::static_pointer_cast<BluetoothHostInterface>(st_it->second);
            return;
        }
        m_device = nullptr;
    };

private:
    void setup();
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
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    void link_device(bool claim_devices) override
    {
        auto &claimed = claim_devices ? m_profile->claimed_devices : m_profile->temp_claimed_devices;
        auto it = claimed.find(m_input.deviceid);
        if (it != claimed.end() && it->second) {
            m_device = std::static_pointer_cast<BluetoothHostInterface>(it->second);
            return;
        }
        if (!claimed.empty()) {
            m_device = std::static_pointer_cast<BluetoothHostInterface>(claimed.begin()->second);
            return;
        }
        auto st_it = m_profile->devices.find(m_input.deviceid);
        if (st_it != m_profile->devices.end()) {
            m_device = std::static_pointer_cast<BluetoothHostInterface>(st_it->second);
            return;
        }
        m_device = nullptr;
    };

private:
    void setup();
    proto_BTAxisInput m_input;
    std::shared_ptr<BluetoothHostInterface> m_device;
    Profile *m_profile;
};

