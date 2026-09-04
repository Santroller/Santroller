#include "devices/bt/bluetooth_stack.hpp"

#include <pico/cyw43_arch.h>

#include "btstack.h"
#include "devices/bt/bt_classic_rx.hpp"
#include "devices/bt/ble_rx.hpp"
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
    if (m_initialized && !m_powered)
    {
        hci_power_control(HCI_POWER_ON);
        m_powered = true;
    }
}

void BluetoothStack::power_off()
{
    if (m_initialized && m_powered)
    {
        hci_power_control(HCI_POWER_OFF);
        m_powered = false;
    }
}

void BluetoothStack::request_wiimote(void *report)
{
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
    m_wiimote_report = report;
    if (m_initialized)
    {
        wiimote_emulator_update_report(report);
    }
}

void BluetoothStack::release_wiimote()
{
    if (m_wiimote_report)
    {
        wiimote_emulator_shutdown();
        m_wiimote_report = nullptr;
    }
}

bool BluetoothStack::initialized() const
{
    return m_initialized;
}
