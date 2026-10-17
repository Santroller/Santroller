#include "devices/bluetooth.hpp"
#include <pico/cyw43_arch.h>
#include "events.pb.h"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "managers/config_manager.hpp"
#include "managers/inactivity_manager.hpp"
#include "managers/profile_manager.hpp"
#include "managers/device_manager.hpp"
#include "devices/bt/bt_classic_rx.hpp"
#include "devices/bt/ble_rx.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "devices/bt/bt_host.hpp"
#include "devices/bt/bluetooth_status.hpp"
BluetoothDevice::BluetoothDevice(proto_BluetoothDevice device, uint16_t id) : Device(id), m_device(device), m_sync(device.has_syncPin ? device.syncPin : -1)
{
}
// No CYW43, so its pins are free again, but the probe drove them. A device set up before the probe on any
// of them has to set up again, which a full reload does (the stack stays up, so the probe isn't repeated)
void BluetoothDevice::set_up_cyw43_pin_users_again()
{
    if (!BluetoothStack::instance().take_cyw43_pins_released())
    {
        return;
    }
    bool clash = false;
    DeviceManager::instance().for_each_active_device([&](const std::shared_ptr<Device> &device) {
        for (uint8_t pin = 0; pin < NUM_BANK0_GPIOS; pin++)
        {
            if (device.get() != this && BluetoothStack::is_cyw43_pin(pin) && device->using_pin(pin))
            {
                clash = true;
            }
        }
    });
    if (clash)
    {
        printf("bt: devices use the CYW43 pins, setting them up again\r\n");
        ConfigManager::instance().set_full_reload(true);
        reload();
    }
}

void BluetoothDevice::begin()
{
    m_sync.begin();
    BluetoothStack::instance().set_device_id(m_id);
    // TODO: if we add support for swapping pins, then we gotta deinit here if the pins change
    if (BluetoothStack::instance().initialized())
    {
        if (!BluetoothStack::instance().is_powered())
        {
            BluetoothStack::instance().power_on();
        }
        // Something else (eg a bluetooth profile) may have started the stack first
        set_up_cyw43_pin_users_again();
        ConfigManager::instance().set_bluetooth_available(BluetoothStack::instance().controller_available());
        return;
    }
    printf("bt device init\r\n");
    if (BluetoothStack::instance().begin())
    {
        set_up_cyw43_pin_users_again();
        BluetoothStack::instance().power_on();
        // Without a CYW43 this waits for a USB adapter, see update()
        ConfigManager::instance().set_bluetooth_available(BluetoothStack::instance().controller_available());
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
    if (first)
    {
        bt_host_add_assignable_devices(true);
    }
}

void BluetoothDevice::end(bool full)
{
    m_sync.end();
    if (m_timed_out)
    {
        // Whatever replaces this device decides whether bluetooth stays off
        m_timed_out = false;
        BluetoothStack::instance().power_on();
    }
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
    // A USB adapter was plugged in or pulled out. Profiles only make their bluetooth instances when
    // they load, so reload for them to pick it up (or drop it)
    bool available = BluetoothStack::instance().controller_available();
    if (available != ConfigManager::instance().has_bluetooth())
    {
        printf("bt controller %s\r\n", available ? "available" : "gone");
        ConfigManager::instance().set_bluetooth_available(available);
        reload();
        full_poll = true;
    }
    // full polls only happen when a tool loads or the profile changes, so host connections and
    // disconnections are sent as they happen too
    bool host_connected = bluetooth_host_connected();
    if (full_poll || host_connected != m_sent_host_connected)
    {
        proto_Event event = proto_Event_init_default;
        event.which_event = proto_Event_device_tag;
        event.event.device.id = m_id;
        event.event.device.connected = ConfigManager::instance().has_bluetooth();
        event.event.device.has_hostConnected = true;
        event.event.device.hostConnected = host_connected;
        // try again next update if no tool is listening
        m_sent_host_connected = HIDConfigDevice::send_event(event, full_poll) ? host_connected : -1;
    }
    bt_host_update_interfaces(full_poll, send_events);
    if (m_sync.pressed())
    {
        InactivityManager::instance().input_activity(millis());
        start_discovery();
    }
    if (m_device.has_timeoutSec && m_device.timeoutSec)
    {
        update_timeout();
    }
}

void BluetoothDevice::update_timeout()
{
    auto &stack = BluetoothStack::instance();
    if (!stack.initialized())
    {
        return;
    }
    bool idle = InactivityManager::instance().idle_ms(millis()) >= m_device.timeoutSec * 1000;
    if (!idle)
    {
        if (m_timed_out)
        {
            printf("bt timeout over, powering on\r\n");
            m_timed_out = false;
            stack.power_on();
        }
        return;
    }
    if (m_timed_out)
    {
        return;
    }
    // Only time out when acting as a bluetooth controller, as inputs on this controller are
    // what turn bluetooth back on, and connected controllers would be cut off
    bool bluetooth_output = false;
    ProfileManager::instance().for_each_instance([&](const std::shared_ptr<Instance> &instance) {
        bluetooth_output |= instance->is_bluetooth();
    });
    if (!bluetooth_output || bt_host_assignable_interface_count())
    {
        return;
    }
    printf("bt timed out, powering off\r\n");
    m_timed_out = true;
    bt_discovery_stop();
    stack.power_off();
}

bool BluetoothDevice::using_pin(uint8_t pin)
{
    return BluetoothStack::instance().using_cyw43_pin(pin) || m_sync.using_pin(pin);
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
        printf("LIAC inquiry complete without new connection, stopping\r\n");
        bt_discovery_stop();
    }
}

void BluetoothDevice::handle_command(proto_Command command)
{
    if (command.which_command == proto_Command_scan_tag)
    {
        start_discovery();
    }
}

void BluetoothDevice::start_discovery()
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