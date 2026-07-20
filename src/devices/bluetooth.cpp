#include "devices/bluetooth.hpp"
#include <pico/cyw43_arch.h>
#include "events.pb.h"
#include "usb/device/hid_device.h"
#include "config.hpp"
static bool bluetooth_initted = false;
static bool init_done = true;
BluetoothDevice::BluetoothDevice(proto_BluetoothDevice device, uint16_t id) : Device(id), m_device(device)
{
}
void BluetoothDevice::begin()
{
    // TODO: if we add support for swapping pins, then we gotta deinit here if the pins change
    if (bluetooth_initted)
    {
        return;
    }
    printf("bt device init %d\r\n", bluetooth_initted);
    if (cyw43_arch_init() == 0)
    {
        isPicoW = true;
        printf("bt device init success\r\n");
    }
    else
    {
        printf("bt device init failed\r\n");
        isPicoW = false;
    }
    bluetooth_initted = true;
}

void BluetoothDevice::end(bool full)
{
    cyw43_arch_deinit();
    bluetooth_initted = false;
}
BluetoothDevice::~BluetoothDevice()
{
}

void BluetoothDevice::update(bool full_poll, bool send_events)
{
    if (full_poll)
    {
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, isPicoW}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool BluetoothDevice::using_pin(uint8_t pin)
{
    return pin == 23 || pin == 24 || pin == 25 || pin == 29;
}