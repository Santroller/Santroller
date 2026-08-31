#include "devices/bluetooth.hpp"
#include <pico/cyw43_arch.h>
#include "events.pb.h"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "managers/config_manager.hpp"
#include "devices/bt/bt_classic_rx.hpp"
#include "devices/bt/ble_rx.hpp"
static bool bluetooth_initted = false;
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
        ConfigManager::instance().set_bluetooth_available(true);
        printf("bt device init success\r\n");
        ble_main();
        btstack_classic_main();
    }
    else
    {
        printf("bt device init failed\r\n");
        ConfigManager::instance().set_bluetooth_available(false);
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
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, ConfigManager::instance().has_bluetooth()}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool BluetoothDevice::using_pin(uint8_t pin)
{
    return pin == 23 || pin == 24 || pin == 25 || pin == 29;
}

void BluetoothDevice::handle_command(proto_Command command)
{
    if (command.which_command == proto_Command_scan_tag) {
        btc_start_scan();
        ble_start_scan();
    }
}