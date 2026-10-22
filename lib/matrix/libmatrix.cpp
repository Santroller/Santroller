#include "libmatrix.hpp"
Matrix::Matrix(uint64_t pins, uint64_t outPins, bool active_low): m_pins(pins), m_outPins(outPins), m_active_low(active_low) {
    gpio_init_mask64(pins);
    gpio_init_mask64(outPins);
    gpio_set_dir_in_masked64(pins);
    for (uint8_t pin = 0; pin < NUM_BANK0_GPIOS; pin++) {
        if (pins & (1ull << pin)) {
            gpio_set_pulls(pin, active_low, !active_low);
        }
    }
    gpio_put_masked64(outPins, active_low ? outPins : 0);
    gpio_set_dir_out_masked64(outPins);
}
bool Matrix::read(uint8_t out_pin, uint8_t in_pin) {
    const uint64_t selected = 1ull << out_pin;
    gpio_put_masked64(m_outPins, m_active_low ? m_outPins & ~selected : selected);
    gpio_set_dir_out_masked64(m_outPins);
    sleep_us(1);
    bool ret = gpio_get(in_pin) != m_active_low;
    gpio_put_masked64(m_outPins, m_active_low ? m_outPins : 0);
    gpio_set_dir_in_masked64(m_outPins);
    return ret;
}