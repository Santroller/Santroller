#pragma once
#include "triggers/base_trigger.hpp"
#include "config.pb.h"
#include <memory>

class UsbModeActivationTrigger : public ActivationTrigger
{
public:
    UsbModeActivationTrigger(proto_UsbDeviceAssignment config, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~UsbModeActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return AssignUsb; }
    bool forcedConsoleMode(ConsoleMode& mode) const;
    bool xinputOnWindows(bool &enabled) const;
    bool ps4OrPs5Mode(bool &enabled) const;

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

class JoybusEmulationActivationTrigger : public ActivationTrigger
{
public:
    JoybusEmulationActivationTrigger(proto_JoybusAssignment config, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~JoybusEmulationActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return AssignJoybus; }

private:
    proto_JoybusAssignment m_config;
};

class SNESEmulationActivationTrigger : public ActivationTrigger
{
public:
    SNESEmulationActivationTrigger(proto_SNESAssignment config, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~SNESEmulationActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return AssignSnes; }

private:
    proto_SNESAssignment m_config;
};

class InputActivationTrigger;
class Input;

class XboxOneAuthActivationTrigger : public ActivationTrigger
{
public:
    XboxOneAuthActivationTrigger(proto_XboxOneAuthAssignment config, std::unique_ptr<Input> input, std::shared_ptr<Profile> profile, uint32_t id, uint32_t slot_id, uint32_t list_id);
    ~XboxOneAuthActivationTrigger();
    bool validate(bool claim_device, bool full_poll, bool send_events);
    void reset();
    int assignedDevices() { return 0; }
    bool holds_xbox_one_auth() const { return true; }

private:
    std::unique_ptr<InputActivationTrigger> m_trigger;
};
