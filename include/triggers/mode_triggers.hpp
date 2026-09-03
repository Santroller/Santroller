#pragma once
#include "triggers/base_trigger.hpp"
#include "config.pb.h"

class UsbModeActivationTrigger : public ActivationTrigger
{
public:
    UsbModeActivationTrigger(proto_UsbDeviceAssignment config, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~UsbModeActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return AssignUsb; }
    bool forcedConsoleMode(ConsoleMode& mode) const;

protected:
    proto_UsbDeviceAssignment m_config;
};

class BluetoothModeActivationTrigger : public ActivationTrigger
{
public:
    BluetoothModeActivationTrigger(proto_BluetoothMode mode, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~BluetoothModeActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return m_mode == BTWiimote ? AssignBluetoothWiimote : AssignBluetoothGamepad; }

private:
    proto_BluetoothMode m_mode;
};

class WiiExtensionEmulationActivationTrigger : public ActivationTrigger
{
public:
    WiiExtensionEmulationActivationTrigger(proto_WiimoteAssignment config, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~WiiExtensionEmulationActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return AssignWiimoteExtension; }

private:
    proto_WiimoteAssignment m_config;
};

class PS2ControllerEmulationActivationTrigger : public ActivationTrigger
{
public:
    PS2ControllerEmulationActivationTrigger(proto_PSXAssignment config, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~PS2ControllerEmulationActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return AssignPsx; }

private:
    proto_PSXAssignment m_config;
};
