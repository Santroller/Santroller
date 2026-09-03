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
    void release_wiimote();
    bool initialized() const;

private:
    BluetoothStack() = default;

    bool m_initialized = false;
    bool m_powered = false;
    void *m_wiimote_report = nullptr;
};
