#pragma once

#include <stdint.h>

// BTstack runs from the CYW43 background IRQ (threadsafe_background), so any BTstack
// call made from the main loop must hold the async context lock. Recursive, so it is
// also fine to take inside BTstack callbacks. Does nothing before the stack is up.
// With a USB bluetooth adapter instead, BTstack is polled from the main loop.
class BtStackLock
{
public:
    BtStackLock();
    ~BtStackLock();
    BtStackLock(const BtStackLock &) = delete;
    BtStackLock &operator=(const BtStackLock &) = delete;

private:
    struct async_context *m_context;
};

class BluetoothStack
{
public:
    static BluetoothStack& instance();

    bool begin();
    void power_on();
    void power_off();
    void request_wiimote(void *report);
    void update_wiimote_report(void *report);
    void release_wiimote();
    bool initialized() const;
    void tick();
    bool is_powered() const { return m_powered; }
    // True when the CYW43 HCI transport is unavailable and a USB adapter on the host port is used.
    bool uses_usb_dongle() const { return m_usb_dongle; }
    // Whether there is a controller to use: the CYW43, or a USB adapter that is actually plugged in.
    // The stack runs without an adapter, waiting for one, but nothing can be done over it until then
    bool controller_available() const;
    // The pins a Pico W's CYW43 is wired to. Only claimed while the CYW43 is in use: on a board without
    // one the probe gives them back, so they're free for anything else
    static bool is_cyw43_pin(uint8_t pin);
    bool using_cyw43_pin(uint8_t pin) const { return m_initialized && !m_usb_dongle && is_cyw43_pin(pin); }
    // True once, after a failed CYW43 probe handed its pins back. Devices set up before the probe had
    // those pins changed under them
    bool take_cyw43_pins_released()
    {
        bool released = m_cyw43_pins_released;
        m_cyw43_pins_released = false;
        return released;
    }
    // The controller's BD_ADDR (most significant byte first), once the stack is up and
    // running. Returns false if bluetooth isn't enabled or isn't ready yet.
    bool local_address(uint8_t addr[6]) const;
    // Whether the USB port has power, which the CYW43 senses on boards like the Pico W. Returns
    // false when that isn't known (no CYW43, or it isn't up yet).
    bool vbus_present(bool &present, uint32_t now);
    uint16_t device_id() const { return m_device_id; }
    void set_device_id(uint16_t device_id) { m_device_id = device_id; }

private:
    BluetoothStack() = default;

    bool m_initialized = false;
    bool m_powered = false;
    bool m_usb_dongle = false;
    bool m_cyw43_pins_released = false;
    bool m_restarting = false;
    void *m_wiimote_report = nullptr;
    uint16_t m_device_id = 0;
    bool m_vbus = false;
    uint32_t m_vbus_read_ms = 0;
};
