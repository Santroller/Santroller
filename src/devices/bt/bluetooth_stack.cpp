#include "devices/bt/bluetooth_stack.hpp"
#include "devices/bt/bluetooth_status.hpp"

#include <pico/cyw43_arch.h>

#include "btstack.h"
#include <string.h>
#include "devices/bt/bt_classic_rx.hpp"
#include "devices/bt/ble_rx.hpp"
#include "devices/bt/bt_tlv_storage.hpp"
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

BtStackLock::BtStackLock() : m_context(cyw43_arch_async_context())
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

    if (cyw43_arch_init() != 0)
    {
        return false;
    }

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
        hci_power_control(HCI_POWER_ON);
        m_powered = true;
    }
}

void BluetoothStack::power_off()
{
    BtStackLock lock;
    if (m_initialized && m_powered)
    {
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
