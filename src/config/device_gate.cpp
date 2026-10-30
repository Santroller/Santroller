#include "config/device_gate.hpp"
#include "devices/pins.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdio.h>

static constexpr uint32_t ADC_SAMPLES = 8;

bool DeviceGates::read(const proto_DeviceGate &gate)
{
    if (gate.pin < 0)
    {
        return false;
    }
    if (gate.analog)
    {
        if (!is_adc_pin(gate.pin))
        {
            return false;
        }
        return device_gate_matches(gate, false, adc_read_pin_mv(gate.pin, ADC_SAMPLES));
    }
    return device_gate_matches(gate, gpio_get(gate.pin), 0);
}

bool DeviceGates::check(uint32_t device_id, const proto_DeviceGate &gate)
{
    for (const auto &state : m_gates)
    {
        if (state.device_id == device_id)
        {
            return state.open;
        }
    }
    if (gate.pin >= 0)
    {
        // Analog gates are switched over to the ADC only while they're read
        gpio_init(gate.pin);
        gpio_set_dir(gate.pin, GPIO_IN);
        gpio_set_pin_mode(gate.pin, gate.pinMode);
        // Let the pull charge whatever is on the pin before reading it
        busy_wait_us(200);
    }
    bool open = read(gate);
    printf("device %d gate pin %d: %s\r\n", (int)device_id, (int)gate.pin, open ? "open" : "closed");
    m_gates.push_back({device_id, gate, open});
    return open;
}

bool DeviceGates::poll(uint32_t now)
{
    // Once a reload is due there's nothing more to see until it reads the pins again
    if (m_reload_due || m_gates.empty() || now - m_last_poll < POLL_MS)
    {
        return false;
    }
    m_last_poll = now;
    for (auto &state : m_gates)
    {
        m_reload_due |= state.update(read(state.gate), now);
    }
    return m_reload_due;
}
