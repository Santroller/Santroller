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