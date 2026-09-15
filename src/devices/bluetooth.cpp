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
    // TODO: if we add support for swapping pins, then we gotta deinit here if the pins change
    if (BluetoothStack::instance().initialized())
    {
        return;
    }
    printf("bt device init\r\n");
    if (BluetoothStack::instance().begin())
    {
        ConfigManager::instance().set_bluetooth_available(true);
        printf("bt device init success\r\n");
    }
    else
    {
        printf("bt device init failed\r\n");
        ConfigManager::instance().set_bluetooth_available(false);
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

namespace {
enum ScanPhase {
    SCAN_PHASE_IDLE,
    SCAN_PHASE_BLE,
    SCAN_PHASE_CLASSIC_GIAC,
    SCAN_PHASE_CLASSIC_LIAC,
};

static ScanPhase s_scan_phase = SCAN_PHASE_IDLE;
static btstack_timer_source_t s_scan_timer;

static void scan_timer_callback(btstack_timer_source_t *ts)
{
    UNUSED(ts);
    switch (s_scan_phase)
    {
    case SCAN_PHASE_BLE:
        if (ble_is_connecting() || ble_has_connected_device())
        {
            printf("BLE device is connecting or connected, stopping discovery scan cycle\r\n");
            bt_discovery_on_device_found();
            break;
        }
        printf("BLE scan period finished, advancing to Classic GIAC inquiry...\r\n");
        ble_stop_scan();
        s_scan_phase = SCAN_PHASE_CLASSIC_GIAC;
        btc_start_scan(GAP_IAC_GENERAL_INQUIRY);
        // Watchdog timer in case inquiry gets stuck
        btstack_run_loop_set_timer(&s_scan_timer, 15000);
        btstack_run_loop_set_timer_handler(&s_scan_timer, scan_timer_callback);
        btstack_run_loop_add_timer(&s_scan_timer);
        break;

    case SCAN_PHASE_CLASSIC_GIAC:
    case SCAN_PHASE_CLASSIC_LIAC:
        printf("Classic inquiry watchdog timeout, ending scan cycle\r\n");
        bt_discovery_stop();
        break;

    default:
        break;
    }
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
    if (s_scan_phase != SCAN_PHASE_IDLE)
    {
        printf("Bluetooth device found/connecting, stopping discovery scan cycle\r\n");
        btstack_run_loop_remove_timer(&s_scan_timer);
        s_scan_phase = SCAN_PHASE_IDLE;
    }
}

void bt_classic_on_inquiry_complete_empty()
{
    if (s_scan_phase == SCAN_PHASE_CLASSIC_GIAC)
    {
        printf("Classic GIAC inquiry found 0 devices, advancing to Classic LIAC inquiry (Wii Sync)...\r\n");
        s_scan_phase = SCAN_PHASE_CLASSIC_LIAC;
        btc_start_scan(GAP_IAC_LIMITED_INQUIRY);
        btstack_run_loop_remove_timer(&s_scan_timer);
        btstack_run_loop_set_timer(&s_scan_timer, 15000);
        btstack_run_loop_set_timer_handler(&s_scan_timer, scan_timer_callback);
        btstack_run_loop_add_timer(&s_scan_timer);
    }
    else if (s_scan_phase == SCAN_PHASE_CLASSIC_LIAC)
    {
        printf("Scan cycle complete\r\n");
        bt_discovery_stop();
    }
}

void BluetoothDevice::handle_command(proto_Command command)
{
    if (command.which_command == proto_Command_scan_tag) {
        printf("Starting Bluetooth discovery scan cycle (powered=%d)...\r\n", (int)BluetoothStack::instance().is_powered());
        bt_discovery_stop();

        s_scan_phase = SCAN_PHASE_BLE;
        ble_start_scan();
        btstack_run_loop_set_timer(&s_scan_timer, 5000);
        btstack_run_loop_set_timer_handler(&s_scan_timer, scan_timer_callback);
        btstack_run_loop_add_timer(&s_scan_timer);
    }
}