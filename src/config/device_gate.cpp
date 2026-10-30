#include "config/device_gate.hpp"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdio.h>

static constexpr uint32_t ADC_PIN_COUNT = 4;
static constexpr uint32_t ADC_SAMPLES = 8;
// The ADC's reference, as on a Pico
static constexpr uint32_t ADC_VREF_MV = 3300;

static bool is_adc_pin(int32_t pin)
{
    return pin >= ADC_BASE_PIN && pin < static_cast<int32_t>(ADC_BASE_PIN + ADC_PIN_COUNT);
}

static void set_pulls(uint pin, PinMode mode)
{
    switch (mode)
    {
    case PinMode_PullUp:
        gpio_set_pulls(pin, true, false);
        break;
    case PinMode_PullDown:
        gpio_set_pulls(pin, false, true);
        break;
    case PinMode_BusKeep:
        gpio_set_pulls(pin, true, true);
        break;
    default:
        gpio_set_pulls(pin, false, false);
        break;
    }
}

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
        adc_select_input(gate.pin - ADC_BASE_PIN);
        // The first read after switching channels can still see the last one
        adc_read();
        uint32_t total = 0;
        for (uint32_t i = 0; i < ADC_SAMPLES; i++)
        {
            total += adc_read();
        }
        return device_gate_matches(gate, false, total * ADC_VREF_MV / (ADC_SAMPLES * 4095));
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
        if (gate.analog && is_adc_pin(gate.pin))
        {
            adc_gpio_init(gate.pin);
        }
        else
        {
            gpio_init(gate.pin);
            gpio_set_dir(gate.pin, GPIO_IN);
        }
        set_pulls(gate.pin, gate.pinMode);
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
