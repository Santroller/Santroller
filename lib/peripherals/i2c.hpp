#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <hardware/i2c.h>
#include <hardware/gpio.h>

class I2CMasterInterface {
   public:
    I2CMasterInterface(uint8_t block, int8_t sda, int8_t scl, uint32_t clock);
    bool readRegister(uint8_t address, uint8_t pointer, uint8_t length,
                         uint8_t *data);
    bool readRegisterRepeatedStart(uint8_t address, uint8_t pointer, uint8_t length,
                                      uint8_t *data);
    bool readRegisterSlow(uint8_t address, uint8_t pointer, uint8_t length,
                             uint8_t *data);
    bool writeRegister(uint8_t address, uint8_t pointer, uint8_t data);
    bool writeRegister(uint8_t address, uint8_t pointer, uint8_t length,
                        uint8_t *data);
    bool readFrom(uint8_t address, uint8_t *data, uint8_t length,
                  uint8_t sendStop);

    bool writeTo(uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
                 uint8_t sendStop);

   private:
    i2c_inst_t *i2c;
    i2c_inst_t* _hardwareBlocks[NUM_I2CS] = {i2c0,i2c1};
};

class I2CSlaveInterface {
   public:
    void init();
    void on_byte_received(void (*handler)(uint8_t addr, uint8_t data));
    void on_data_received_end(void (*handler)(uint8_t addr, uint8_t data));
    void on_data_requested(uint8_t (*handler)(uint8_t addr));
};