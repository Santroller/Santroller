#include "devices/bluetooth.hpp"
#include <pico/cyw43_arch.h>
#include "events.pb.h"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "managers/config_manager.hpp"
#include "devices/bt/bt_classic_rx.hpp"
#include "devices/bt/ble_rx.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "devices/bt/bt_host.hpp"
BluetoothDevice::BluetoothDevice(proto_BluetoothDevice device, uint16_t id) : Device(id), m_device(device)
{
}
void BluetoothDevice::begin()
{
    BluetoothStack::instance().set_device_id(m_id);
    // TODO: if we add support for swapping pins, then we gotta deinit here if the pins change
    if (BluetoothStack::instance().initialized())
    {
        if (!BluetoothStack::instance().is_powered())
        {
            BluetoothStack::instance().power_on();
        }
        return;
    }
    printf("bt device init\r\n");
    if (BluetoothStack::instance().begin())
    {
        BluetoothStack::instance().power_on();
        ConfigManager::instance().set_bluetooth_available(true);
        printf("bt device init success\r\n");
    }
    else
    {
        printf("bt device init failed\r\n");
        ConfigManager::instance().set_bluetooth_available(false);
    }
}

void BluetoothDevice::rescan(bool first)
{
    printf("BluetoothDevice rescan\r\n");
    if (first)
    {
        printf("assignable_devices before: %d\r\n", bt_host_assignable_interface_count());

        bt_host_add_assignable_devices(true);
        printf("assignable_devices after: %d\r\n", bt_host_assignable_interface_count());
    }
}

void BluetoothDevice::end(bool full)
{
    if (full)
    {
        BluetoothStack::instance().power_off();
        ConfigManager::instance().set_bluetooth_available(false);
    }
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
    bt_host_update_interfaces(full_poll, send_events);
}

bool BluetoothDevice::using_pin(uint8_t pin)
{
    return pin == 23 || pin == 24 || pin == 25 || pin == 29;
}

#include "bluetooth.h"
#include "btstack_defines.h"
#include "btstack_run_loop.h"

namespace
{
    enum ScanPhase
    {
        SCAN_PHASE_IDLE,
        SCAN_PHASE_BLE,
        SCAN_PHASE_CLASSIC_GIAC,
        SCAN_PHASE_CLASSIC_LIAC,
    };

    static ScanPhase s_scan_phase = SCAN_PHASE_IDLE;
    static btstack_timer_source_t s_scan_timer;

    static void scan_timer_callback(btstack_timer_source_t *ts)
    {
        bt_discovery_stop();
    }
}

void bt_discovery_stop()
{
    btstack_run_loop_remove_timer(&s_scan_timer);
    s_scan_phase = SCAN_PHASE_IDLE;
    ble_stop_scan();
    btc_stop_scan();
}

void bt_discovery_on_device_found()
{
    bt_discovery_stop();
}

void bt_classic_on_inquiry_complete_empty()
{
    if (s_scan_phase == SCAN_PHASE_CLASSIC_GIAC)
    {
        s_scan_phase = SCAN_PHASE_CLASSIC_LIAC;
        printf("GIAC inquiry complete without new connection, trying LIAC...\r\n");
        btc_start_scan(GAP_IAC_LIMITED_INQUIRY);
    }
    else if (s_scan_phase == SCAN_PHASE_CLASSIC_LIAC)
    {
        s_scan_phase = SCAN_PHASE_CLASSIC_GIAC;
        printf("LIAC inquiry complete without new connection, cycling back to GIAC...\r\n");
        btc_start_scan(GAP_IAC_GENERAL_INQUIRY);
    }
}

void BluetoothDevice::handle_command(proto_Command command)
{
    if (command.which_command == proto_Command_scan_tag)
    {
        printf("Starting Bluetooth discovery scan cycle (powered=%d)...\r\n", (int)BluetoothStack::instance().is_powered());
        bt_discovery_stop();

        s_scan_phase = SCAN_PHASE_CLASSIC_GIAC;
        btc_start_scan(GAP_IAC_GENERAL_INQUIRY);
        ble_start_scan();

        btstack_run_loop_set_timer(&s_scan_timer, 25000);
        btstack_run_loop_set_timer_handler(&s_scan_timer, scan_timer_callback);
        btstack_run_loop_add_timer(&s_scan_timer);
    }
}