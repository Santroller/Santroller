#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "libxbox360_rf.hpp"
#include "sync_button.hpp"

// An Xbox 360 RF module wired up as a wireless receiver. Controller inputs arrive over
// USB host like any other receiver, this just initialises the module and sends sync.
class Xbox360RfDevice : public Device
{
public:
    ~Xbox360RfDevice() {}
    Xbox360RfDevice(proto_Xbox360RfDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    // The config tool's scan button syncs, like it starts pairing for Bluetooth
    void handle_command(proto_Command command);

private:
    proto_Xbox360RfDevice m_device;
    Xbox360Rf m_rf;
    SyncButton m_sync;
};
