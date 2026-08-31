#pragma once
#include "triggers/base_trigger.hpp"
#include "config.pb.h"

class WiiExtTypeActivationTrigger : public ActivationTrigger
{
public:
    WiiExtTypeActivationTrigger(proto_WiiExtType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~WiiExtTypeActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return 0; }

protected:
    proto_WiiExtType m_type;
};

class PS2ControllerTypeActivationTrigger : public ActivationTrigger
{
public:
    PS2ControllerTypeActivationTrigger(proto_PS2ControllerType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~PS2ControllerTypeActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return 0; }

protected:
    proto_PS2ControllerType m_type;
};

class UsbTypeActivationTrigger : public ActivationTrigger
{
public:
    UsbTypeActivationTrigger(proto_SubType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~UsbTypeActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return 0; }

protected:
    proto_SubType m_type;
};

class SpecificUsbDeviceActivationTrigger : public ActivationTrigger
{
public:
    SpecificUsbDeviceActivationTrigger(proto_SpecificUsbDevice device, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~SpecificUsbDeviceActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return 0; }

protected:
    proto_SpecificUsbDevice m_device;
};

class BluetoothTypeActivationTrigger : public ActivationTrigger
{
public:
    BluetoothTypeActivationTrigger(proto_SubType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~BluetoothTypeActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return 0; }

protected:
    proto_SubType m_type;
};

class SpecificBluetoothDeviceActivationTrigger : public ActivationTrigger
{
public:
    SpecificBluetoothDeviceActivationTrigger(proto_SpecificUsbDevice device, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~SpecificBluetoothDeviceActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return 0; }

protected:
    proto_SpecificUsbDevice m_device;
};

class MidiChannelActivationTrigger : public ActivationTrigger
{
public:
    MidiChannelActivationTrigger(uint32_t channel, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~MidiChannelActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return 0; }

protected:
    uint32_t m_channel;
};
