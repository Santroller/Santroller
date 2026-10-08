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
    // True when there is no CYW43 (e.g. a Pico instead of a Pico W) and a USB bluetooth adapter on the
    // host port is used instead
    bool uses_usb_dongle() const { return m_usb_dongle; }
    // The controller's BD_ADDR (most significant byte first), once the stack is up and
    // running. Returns false if bluetooth isn't enabled or isn't ready yet.
    bool local_address(uint8_t addr[6]) const;
    uint16_t device_id() const { return m_device_id; }
    void set_device_id(uint16_t device_id) { m_device_id = device_id; }

private:
    BluetoothStack() = default;

    bool m_initialized = false;
    bool m_powered = false;
    bool m_usb_dongle = false;
    bool m_restarting = false;
    void *m_wiimote_report = nullptr;
    uint16_t m_device_id = 0;
};
