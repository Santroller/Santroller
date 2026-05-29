#include "libmatrix.hpp"
Matrix::Matrix(uint32_t pins, uint32_t outPins): m_pins(pins), m_outPins(outPins) {
    gpio_init_mask(pins);
    gpio_init_mask(outPins);
    gpio_set_dir_in_masked(pins);
    gpio_set_dir_out_masked(outPins);
}
bool Matrix::read(uint32_t outPin, uint8_t in_pin) {
    gpio_put_masked(m_outPins, 1 << outPin);
    sleep_us(1);
    uint8_t ret = gpio_get(in_pin);
    gpio_put_masked(m_outPins, 0);
    return ret;
}