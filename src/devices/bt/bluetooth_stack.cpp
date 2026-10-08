#include "devices/bt/bluetooth_stack.hpp"
#include "devices/bt/bluetooth_status.hpp"

#include <pico/cyw43_arch.h>
#include <pico/async_context_poll.h>
#include "pico/btstack_run_loop_async_context.h"

#include "btstack.h"
#ifdef BT_HCI_DUMP
#include "hci_dump_embedded_stdout.h"
#endif
#include <string.h>
#include <stdio.h>
#include "devices/bt/bt_classic_rx.hpp"
#include "devices/bt/ble_rx.hpp"
#include "devices/bt/bt_tlv_storage.hpp"
#include "devices/bt/usb_dongle_transport.hpp"
#include "classic/btstack_link_key_db_tlv.h"
#include "ble/le_device_db_tlv.h"
extern "C"
{
#include "wiimote_btstack.h"
}

namespace
{
void wiimote_led_on()
{
}

void wiimote_led_off()
{
}
}

// The context BTstack runs on: the CYW43's, or our own when using a USB bluetooth adapter
static async_context_t *s_context = nullptr;
static async_context_poll_t s_poll_context;

BtStackLock::BtStackLock() : m_context(s_context)
{
    if (m_context)
    {
        async_context_acquire_lock_blocking(m_context);
    }
}

BtStackLock::~BtStackLock()
{
    if (m_context)
    {
        async_context_release_lock(m_context);
    }
}

BluetoothStack& BluetoothStack::instance()
{
    static BluetoothStack stack;
    return stack;
}

bool BluetoothStack::begin()
{
    if (m_initialized)
    {
        return true;
    }

#ifdef BT_FORCE_USB_DONGLE
    if (false)
#else
    if (cyw43_arch_init() == 0)
#endif
    {
        s_context = cyw43_arch_async_context();
    }
    else
    {
        // No CYW43, so use a USB bluetooth adapter on the host port instead. BTstack gets its own
        // context, polled from the main loop like TinyUSB, so the two never run at the same time.
        printf("BT: no CYW43, using a USB bluetooth adapter instead\r\n");
        if (!async_context_poll_init_with_defaults(&s_poll_context))
        {
            printf("BT: couldn't create the bluetooth context\r\n");
            return false;
        }
        s_context = &s_poll_context.core;
        m_usb_dongle = true;
        btstack_memory_init();
        btstack_run_loop_init(btstack_run_loop_async_context_get_instance(s_context));
        hci_init(usb_dongle_transport_instance(), NULL);
        // BTstack's own hardware error handling leaves HCI off, so restart it properly instead
        hci_set_hardware_error_callback(usb_dongle_hardware_error);
    }
    printf("BT: stack initialised\r\n");
#ifdef BT_HCI_DUMP
    // very verbose, logs every HCI packet, including the controller's startup sequence
    hci_dump_init(hci_dump_embedded_stdout_get_instance());
#endif

    l2cap_init();

    // Setup persistent TLV storage for BTstack bonding
    const btstack_tlv_t *tlv_impl = BtTlvStorage::instance().btstack_tlv();
    void *tlv_context = &BtTlvStorage::instance();
    btstack_tlv_set_instance(tlv_impl, tlv_context);
    const btstack_link_key_db_t *link_key_db = btstack_link_key_db_tlv_get_instance(tlv_impl, tlv_context);
    hci_set_link_key_db(link_key_db);
    le_device_db_tlv_configure(tlv_impl, tlv_context);

    ble_main();
    btstack_classic_main(true);

    if (m_wiimote_report)
    {
        wiimote_emulator_set_led(wiimote_led_on, wiimote_led_off);
        wiimote_emulator(m_wiimote_report);
    }

    m_initialized = true;
    return true;
}

void BluetoothStack::power_on()
{
    BtStackLock lock;
    if (m_initialized && !m_powered)
    {
        int err = hci_power_control(HCI_POWER_ON);
        printf("BT: power on (%d)\r\n", err);
        m_powered = true;
    }
}

void BluetoothStack::power_off()
{
    BtStackLock lock;
    if (m_initialized && m_powered)
    {
        printf("BT: power off\r\n");
        hci_power_control(HCI_POWER_OFF);
        m_powered = false;
    }
}

void BluetoothStack::request_wiimote(void *report)
{
    BtStackLock lock;
    m_wiimote_report = report;
    btstack_classic_set_accept_incoming(false);
    if (m_initialized)
    {
        wiimote_emulator_set_led(wiimote_led_on, wiimote_led_off);
        wiimote_emulator(report);
    }
}

void BluetoothStack::update_wiimote_report(void *report)
{
    BtStackLock lock;
    m_wiimote_report = report;
    if (m_initialized)
    {
        wiimote_emulator_update_report(report);
    }
}

void BluetoothStack::release_wiimote()
{
    BtStackLock lock;
    if (m_wiimote_report)
    {
        wiimote_emulator_shutdown();
        m_wiimote_report = nullptr;
        btstack_classic_set_accept_incoming(true);
    }
}

bool BluetoothStack::initialized() const
{
    return m_initialized;
}

bool BluetoothStack::local_address(uint8_t addr[6]) const
{
    if (!m_initialized || !m_powered || hci_get_state() != HCI_STATE_WORKING)
    {
        return false;
    }
    bd_addr_t local;
    gap_local_bd_addr(local);
    memcpy(addr, local, sizeof(local));
    return true;
}
void BluetoothStack::tick() {
    if (m_initialized)
    {
        if (m_usb_dongle)
        {
            async_context_poll(s_context);
            BtStackLock lock;
            if (usb_dongle_take_restart())
            {
                m_restarting = true;
            }
            // Restart HCI: power off (which gives up on an unplugged adapter after a second), then once
            // it is off, power on again so it waits for an adapter to reset
            if (m_restarting && m_powered)
            {
                if (hci_get_state() == HCI_STATE_OFF)
                {
                    printf("BT dongle: restarting bluetooth\r\n");
                    hci_power_control(HCI_POWER_ON);
                    m_restarting = false;
                }
                else if (hci_get_state() != HCI_STATE_HALTING)
                {
                    hci_power_control(HCI_POWER_OFF);
                }
            }
        }
        // log HCI state changes from here, so they show up even if nothing was registered for the event yet
        static HCI_STATE last_state = HCI_STATE_OFF;
        static bool logged = false;
        HCI_STATE state;
        {
            BtStackLock lock;
            state = hci_get_state();
        }
        if (!logged || state != last_state)
        {
            static const char *const names[] = {"off", "initializing", "working", "halting", "sleeping", "falling asleep"};
            printf("BT: HCI state %s, powered %d\r\n", state < 6 ? names[state] : "?", m_powered);
            if (state == HCI_STATE_WORKING)
            {
                bd_addr_t local;
                {
                    BtStackLock lock;
                    gap_local_bd_addr(local);
                }
                printf("BT: address %s\r\n", bd_addr_to_str(local));
            }
            last_state = state;
            logged = true;
        }
        // creating hosts, sending reports etc. all go through BTstack
        BtStackLock lock;
        btc_tick();
        ble_tick();
    }
}

bool bluetooth_connected()
{
    if (!BluetoothStack::instance().initialized())
    {
        return false;
    }
    BtStackLock lock;
    return bt_gamepad_connected() || wiimote_emulator_connected() || btc_has_connected_device() || ble_has_connected_device();
}
