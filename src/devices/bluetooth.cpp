#include "devices/bluetooth.hpp"
#include <pico/cyw43_arch.h>
#include "events.pb.h"
#include "usb/device/hid_device.h"
BluetoothDevice::BluetoothDevice(proto_BluetoothDevice device, uint16_t id): Device(id), m_device(device) {
    if (cyw43_arch_init())
    {
        m_isPicoW = false;
    }
}

BluetoothDevice::~BluetoothDevice() {

}

void BluetoothDevice::update(bool full_poll, bool send_events)
{
    if (full_poll)
    {
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_isPicoW}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool BluetoothDevice::using_pin(uint8_t pin)
{
    return pin == 23 || pin == 24 || pin == 25 || pin == 29;
}