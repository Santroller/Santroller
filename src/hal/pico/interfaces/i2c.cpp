
#include <hardware/i2c.h>

#include <interfaces/core.hpp>
#include <hal/pico/interfaces/i2c.hpp>

#include "hardware/gpio.h"

bool PicoI2CMasterInterface::readFrom(uint8_t address, uint8_t *data, uint8_t length,
                  uint8_t sendStop) {
    int ret =
        i2c_read_timeout_us(i2c, address, data, length, !sendStop, 5000);
    return ret > 0 ? ret : 0;
}

bool PicoI2CMasterInterface::writeTo(uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
                 uint8_t sendStop) {
    int ret =
        i2c_write_timeout_us(i2c, address, data, length, !sendStop, 5000);
    if (ret < 0)
        ret = i2c_write_timeout_us(i2c, address, data, length, !sendStop,
                                   5000);
    return ret > 0;
}