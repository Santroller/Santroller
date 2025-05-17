#include "matrix.hpp"
Matrix::Matrix(uint8_t pin, uint8_t outPin): pin(pin), outPin(outPin) {
    gpio_init(pin);
    gpio_init(outPin);
    gpio_set_dir(pin, false);
    gpio_set_dir(outPin, true);
}
bool Matrix::read() {
    gpio_put(outPin, 0);
    sleep_us(1);
    uint8_t ret = gpio_get(pin);
    gpio_put(outPin, 1);
    return ret;
}