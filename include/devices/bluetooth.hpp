#pragma once
#include "base.hpp"
#include "device.pb.h"
class BluetoothDevice : public Device
{
public:
    ~BluetoothDevice();
    BluetoothDevice(proto_BluetoothDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    void handle_command(proto_Command command);

private:
    proto_BluetoothDevice m_device;
};

void bt_discovery_stop();
void bt_discovery_on_device_found();
void bt_classic_on_inquiry_complete_empty();