#pragma once

#include <stdint.h>

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
    bool is_powered() const { return m_powered; }
    uint16_t device_id() const { return m_device_id; }
    void set_device_id(uint16_t device_id) { m_device_id = device_id; }

private:
    BluetoothStack() = default;

    bool m_initialized = false;
    bool m_powered = false;
    void *m_wiimote_report = nullptr;
    uint16_t m_device_id = 0;
};
