#pragma once
#include "base.hpp"
#include "device.pb.h"
extern bool isPicoW;
class BluetoothDevice : public Device
{
public:
    ~BluetoothDevice();
    BluetoothDevice(proto_BluetoothDevice device, uint16_t id);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);

private:
    proto_BluetoothDevice m_device;
};