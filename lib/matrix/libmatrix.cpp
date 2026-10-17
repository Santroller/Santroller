#include "libmatrix.hpp"
Matrix::Matrix(uint64_t pins, uint64_t outPins): m_pins(pins), m_outPins(outPins) {
    gpio_init_mask64(pins);
    gpio_init_mask64(outPins);
    gpio_set_dir_in_masked64(pins);
    gpio_set_dir_out_masked64(outPins);
}
bool Matrix::read(uint8_t out_pin, uint8_t in_pin) {
    gpio_set_dir_out_masked64(m_outPins);
    gpio_put_masked64(m_outPins, 1ull << out_pin);
    sleep_us(1);
    uint8_t ret = gpio_get(in_pin);
    gpio_put_masked64(m_outPins, 0);
    gpio_set_dir_in_masked64(m_outPins);
    return ret;
}