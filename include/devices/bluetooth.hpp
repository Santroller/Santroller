#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "sync_button.hpp"
class BluetoothDevice : public Device
{
public:
    ~BluetoothDevice();
    BluetoothDevice(proto_BluetoothDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void rescan(bool first);
    bool matches_reload_config(const proto_Device &config) const override
    {
        return config.which_device == proto_Device_bt_tag &&
               config.device.bt.has_syncPin == m_device.has_syncPin &&
               (!m_device.has_syncPin || config.device.bt.syncPin == m_device.syncPin);
    }
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    void handle_command(proto_Command command);
    bool valid()
    {
        return false;
    }

private:
    void start_discovery();
    proto_BluetoothDevice m_device;
    SyncButton m_sync;
};

void bt_discovery_stop();
void bt_discovery_on_device_found();
void bt_classic_on_inquiry_complete_empty();