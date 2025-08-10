#include "i2c.hpp"

#include <pico/time.h>
#include <stdint.h>
#include <string.h>
I2CMasterInterface::I2CMasterInterface(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock) {
    i2c = _hardwareBlocks[block];
    i2c_init(i2c, clock);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);

    gpio_pull_up(sda);
    gpio_pull_up(scl);
}
bool I2CMasterInterface::readRegisterSlow(uint8_t address, uint8_t pointer, uint8_t length,
                                             uint8_t *data) {
    if (!writeTo(address, &pointer, 1, true, true)) return false;
    sleep_us(170);
    return readFrom(address, data, length, true);
}

bool I2CMasterInterface::readRegister(uint8_t address, uint8_t pointer, uint8_t length,
                                         uint8_t *data) {
    return writeTo(address, &pointer, 1, true, true) &&
           readFrom(address, data, length, true);
}
bool I2CMasterInterface::readRegisterRepeatedStart(uint8_t address, uint8_t pointer, uint8_t length,
                                                      uint8_t *data) {
    return writeTo(address, &pointer, 1, true, false) &&
           readFrom(address, data, length, true);
}
bool I2CMasterInterface::writeRegister(uint8_t address, uint8_t pointer, uint8_t data) {
    return writeRegister(address, pointer, 1, &data);
}
bool I2CMasterInterface::writeRegister(uint8_t address, uint8_t pointer, uint8_t length,
                                        uint8_t *data) {
    uint8_t data2[length + 1];
    data2[0] = pointer;
    memcpy(data2 + 1, data, length);

    return writeTo(address, data2, length + 1, true, true);
}

bool I2CMasterInterface::readFrom(uint8_t address, uint8_t *data, uint8_t length,
                                  uint8_t sendStop) {
    int ret =
        i2c_read_timeout_us(i2c, address, data, length, !sendStop, 5000);
    return ret > 0 ? ret : 0;
}

bool I2CMasterInterface::writeTo(uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
                                 uint8_t sendStop) {
    int ret =
        i2c_write_timeout_us(i2c, address, data, length, !sendStop, 5000);
    if (ret < 0)
        ret = i2c_write_timeout_us(i2c, address, data, length, !sendStop,
                                   5000);
    return ret > 0;
}